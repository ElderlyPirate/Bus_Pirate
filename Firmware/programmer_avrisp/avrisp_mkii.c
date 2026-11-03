/**
 * @file avrisp_mkii.c
 *
 * @brief AVRISP-MKII personality - transports and mode plumbing.
 *
 * On the BPv4 the mkII USB personality re-enumerates the device as an
 * Atmel AVRISP mkII compatible vendor-class device (VID/PID 03EB:2104)
 * with the protocol engine talking over bulk endpoint 2, matching
 * avrdude's AVRISP mkII transport (raw command payloads, transfers
 * terminated by a short packet or ZLP). A framed STK500v2 fallback
 * (AVR068) is available over the BPv4 USB CDC serial link.
 *
 * The protocol engine is a port of LUFA's AVRISP-MKII project
 * (Copyright (C) Dean Camera, 2021; MIT-style licence, see
 * Documentation/avrisp-mkii-license.txt).
 */

#include <stdbool.h>

#include "base.h"
#include "avrisp_mkii.h"
#include "avrisp_v2.h"
#include "avrisp_xprog.h"

#ifdef BP_ENABLE_AVRISP_MKII_SUPPORT

/* The CDC descriptor arrays are defined in descriptors.h, which is
   included by main.c.  Declaring them here avoids a second definition. */
extern ROMPTR const unsigned char cdc_device_descriptor[];
extern ROMPTR const unsigned char cdc_config_descriptor[];
extern ROMPTR const unsigned char cdc_str_descs[];
#define AVRISP_CDC_NUM_STRINGS 4u

/** Size of the mkII USB bulk endpoint packets. */
#define AVRISP_USB_PACKET_SIZE 64

/** Detach time for a USB re-enumeration, in milliseconds. */
#define AVRISP_USB_DETACH_MS 100

/** Request payload buffer (also the serial frame reassembly buffer). */
uint8_t avrisp_command_buffer[AVRISP_BUFFER_SIZE];

/** Response buffer. */
uint8_t avrisp_response_buffer[AVRISP_BUFFER_SIZE];

/** Command watchdog, decremented every 10 ms while the mode is active. */
volatile uint8_t avrisp_timeout_ticks;

/** Idle watchdog, decremented every 10 ms while the mode is active. */
static volatile uint16_t avrisp_activity_ticks;

/** Active transport. */
static avrisp_transport_kind_t avrisp_transport;

/** Number of bytes in the response being built. */
static uint16_t avrisp_response_length;

static uint16_t avrisp_request_length;
static uint16_t avrisp_request_position;

/* -------------------------------------------------------------------------
 * Mode timer: 10 ms tick for the command and idle watchdogs.
 * ------------------------------------------------------------------------- */

#pragma interrupt _T3Interrupt

void __attribute__((interrupt, no_auto_psv)) _T3Interrupt(void) {
  if (avrisp_timeout_ticks) {
    avrisp_timeout_ticks--;
  }
  if (avrisp_activity_ticks) {
    avrisp_activity_ticks--;
  }
  IFS0bits.T3IF = 0;
}

static void avrisp_timer_start(void) {
  /* Timer 3, 1:256 prescaler (62.5 kHz): 625 counts are 10 ms. */
  T3CON = 0x0000;
  TMR3 = 0x0000;
  PR3 = 624;
  IPC2bits.T3IP = 4;
  T3CONbits.TCKPS = 0b11;
  IFS0bits.T3IF = 0;
  IEC0bits.T3IE = 1;
  T3CONbits.TON = ON;
}

static void avrisp_timer_stop(void) {
  T3CONbits.TON = OFF;
  IEC0bits.T3IE = 0;
  T3CON = 0x0000;
}

/* -------------------------------------------------------------------------
 * mkII USB personality (BPv4 only).
 * ------------------------------------------------------------------------- */


/** True while the mkII USB personality is enumerated. */
volatile bool avrisp_usb_personality_active;

extern volatile uint8_t usb_device_state;

/* Device descriptor: vendor class, VID/PID 03EB:2104 (Atmel AVRISP mkII). */
static ROMPTR const unsigned char avrisp_device_descriptor[] = {
    0x12,                        /* bLength */
    1u,                          /* bDescriptorType (Device) */
    0x10, 0x01,                  /* bcdUSB 1.10 */
    0xFF,                        /* bDeviceClass (vendor specific) */
    0x00,                        /* bDeviceSubClass */
    0x00,                        /* bDeviceProtocol */
    USB_EP0_BUFFER_SIZE,         /* bMaxPacketSize0 */
    0xEB, 0x03,                  /* idVendor  (0x03EB) */
    0x04, 0x21,                  /* idProduct (0x2104) */
    0x00, 0x02,                  /* bcdDevice 2.00 */
    1u,                          /* iManufacturer */
    2u,                          /* iProduct */
    3u,                          /* iSerialNumber */
    1u                           /* bNumConfigurations */
};

/* Configuration: one vendor interface, bulk EP2 OUT + EP2 IN, 64 bytes. */
static ROMPTR const unsigned char avrisp_config_descriptor[] = {
    0x09,                        /* bLength */
    2u,                          /* bDescriptorType (Configuration) */
    0x20, 0x00,                  /* wTotalLength (32) */
    1u,                          /* bNumInterfaces */
    1u,                          /* bConfigurationValue */
    0u,                          /* iConfiguration */
    0x80,                        /* bmAttributes (bus powered) */
    0x32,                        /* bMaxPower (100 mA) */

    0x09,                        /* bLength */
    4u,                          /* bDescriptorType (Interface) */
    0u,                          /* bInterfaceNumber */
    0u,                          /* bAlternateSetting */
    2u,                          /* bNumEndpoints */
    0xFF,                        /* bInterfaceClass (vendor specific) */
    0x00,                        /* bInterfaceSubClass */
    0x00,                        /* bInterfaceProtocol */
    0u,                          /* iInterface */

    0x07,                        /* bLength */
    5u,                          /* bDescriptorType (Endpoint) */
    0x02,                        /* bEndpointAddress (OUT 2) */
    0x02,                        /* bmAttributes (bulk) */
    0x40, 0x00,                  /* wMaxPacketSize (64) */
    0x00,                        /* bInterval */

    0x07,                        /* bLength */
    5u,                          /* bDescriptorType (Endpoint) */
    0x82,                        /* bEndpointAddress (IN 2) */
    0x02,                        /* bmAttributes (bulk) */
    0x40, 0x00,                  /* wMaxPacketSize (64) */
    0x00                         /* bInterval */
};

/* String descriptors (index 0 is the language). */
static ROMPTR const unsigned char avrisp_string_descriptors[] = {
    4, 3u, 0x09, 0x04,
    12, 3u, 'A', 0, 'T', 0, 'M', 0, 'E', 0, 'L', 0,
    24, 3u, 'A', 0, 'V', 0, 'R', 0, 'I', 0, 'S', 0, 'P', 0, ' ', 0,
            'm', 0, 'k', 0, 'I', 0, 'I', 0,
    /* The real AVRISP mkII serial string contains an embedded NUL. */
    28, 3u, '0', 0, '0', 0, '0', 0, '2', 0, '0', 0, '0', 0, '0', 0,
            '1', 0, '2', 0, '3', 0, '4', 0, '5', 0, 0, 0};

#define AVRISP_NUM_STRINGS 4

static uint8_t avrisp_usb_out_buffer[AVRISP_USB_PACKET_SIZE];
static uint8_t avrisp_usb_in_buffer[AVRISP_USB_PACKET_SIZE];
static volatile BDentry *volatile avrisp_ep_out;
static volatile BDentry *volatile avrisp_ep_in;
static bool avrisp_ep_out_owned;
static uint16_t avrisp_ep_out_length;

void avrisp_usb_configure_endpoints(void) {
  usb_unset_in_handler(1);
  usb_unset_out_handler(1);
  usb_unset_in_handler(2);
  usb_unset_out_handler(2);

  USB_UEP1 = 0;
  USB_UEP2 = USB_EP_INOUT;

  avrisp_ep_out = &usb_bdt[USB_CALC_BD(2, USB_DIR_OUT, USB_PP_EVEN)];
  avrisp_ep_in = &usb_bdt[USB_CALC_BD(2, USB_DIR_IN, USB_PP_EVEN)];

  avrisp_ep_out->BDADDR = avrisp_usb_out_buffer;
  avrisp_ep_out->BDCNT = AVRISP_USB_PACKET_SIZE;
  avrisp_ep_out->BDSTAT = UOWN + DTSEN;

  avrisp_ep_in->BDADDR = avrisp_usb_in_buffer;
  avrisp_ep_in->BDCNT = 0;
  avrisp_ep_in->BDSTAT = DTS + DTSEN;

  avrisp_ep_out_owned = false;
  avrisp_ep_out_length = 0;
}

/** Re-arms the OUT endpoint, dropping any bytes left in the buffer. */
static void avrisp_usb_release_packet(void) {
  if (!avrisp_ep_out_owned) {
    return;
  }

  avrisp_ep_out->BDCNT = AVRISP_USB_PACKET_SIZE;
  avrisp_ep_out->BDSTAT =
      ((avrisp_ep_out->BDSTAT ^ DTS) & DTS) | (UOWN + DTSEN);

  avrisp_ep_out_owned = false;
  avrisp_ep_out_length = 0;
}

/** Waits for the next OUT packet, bounded by the command watchdog. */
static bool avrisp_usb_wait_packet(void) {
  if (usb_device_state < CONFIGURED_STATE) {
    return false;
  }
  while (avrisp_ep_out->BDSTAT & UOWN) {
    if (!avrisp_timeout_ticks || usb_device_state < CONFIGURED_STATE) {
      return false;
    }
  }

  if (usb_device_state < CONFIGURED_STATE) {
    return false;
  }

  avrisp_ep_out_owned = true;
  avrisp_ep_out_length = avrisp_ep_out->BDCNT;
  return true;
}

static bool avrisp_usb_wait_in(void) {
  while (avrisp_ep_in->BDSTAT & UOWN) {
    if (!avrisp_timeout_ticks || usb_device_state < CONFIGURED_STATE) {
      return false;
    }
  }
  return usb_device_state >= CONFIGURED_STATE;
}

static bool avrisp_usb_send_packet(const uint16_t position,
                                  const uint16_t length) {
  if (!avrisp_usb_wait_in()) {
    return false;
  }

  /* The SIE owns both the descriptor AND its buffer until UOWN clears. */
  memcpy(avrisp_usb_in_buffer, &avrisp_response_buffer[position], length);

  avrisp_ep_in->BDCNT = length;
  avrisp_ep_in->BDSTAT = ((avrisp_ep_in->BDSTAT ^ DTS) & DTS) | (UOWN + DTSEN);
  return true;
}

static void avrisp_usb_flush_response(void) {
  uint16_t length = avrisp_response_length;
  uint16_t position = 0;

  while ((length - position) >= AVRISP_USB_PACKET_SIZE) {
    if (!avrisp_usb_send_packet(position, AVRISP_USB_PACKET_SIZE)) {
      return;
    }
    position += AVRISP_USB_PACKET_SIZE;
  }

  if (length > position) {
    if (!avrisp_usb_send_packet(position, length - position)) {
      return;
    }
  }

  /* Terminate transfers that are an exact packet multiple with a ZLP. */
  if (length && ((length % AVRISP_USB_PACKET_SIZE) == 0)) {
    (void)avrisp_usb_send_packet(0, 0);
  }

  (void)avrisp_usb_wait_in();
  avrisp_response_length = 0;
}

static bool avrisp_usb_read_request(void) {
  avrisp_request_length = 0;
  avrisp_request_position = 0;
  for (;;) {
    if (!avrisp_usb_wait_packet()) {
      return false;
    }
    uint16_t length = avrisp_ep_out_length;
    if (length > AVRISP_USB_PACKET_SIZE) {
      return false;
    }
    if (avrisp_request_length + length <= AVRISP_BUFFER_SIZE) {
      memcpy(&avrisp_command_buffer[avrisp_request_length],
             avrisp_usb_out_buffer, length);
      avrisp_request_length += length;
    } else {
      /* Keep draining to the actual short packet/ZLP, without wrapping. */
      avrisp_request_length = AVRISP_BUFFER_SIZE + 1;
    }
    avrisp_usb_release_packet();
    if (length < AVRISP_USB_PACKET_SIZE) {
      return true;
    }
  }
}

static void avrisp_usb_detach(void) {
  DisableGlobalUsbInterrupt();
  U1CONbits.USBEN = 0;
  bp_delay_ms(AVRISP_USB_DETACH_MS);
}

/* usb_start() has an unbounded SE0 wait. Keep USB IRQs masked until the
   line leaves reset, but allow Timer 3 to bound a host-held reset. */
static bool avrisp_usb_attach(void) {
  EnableUsb();
  usb_device_state = ATTACHED_STATE;
  avrisp_timeout_ticks = AVRISP_TIMEOUT_TICKS;
  while (SingleEndedZeroIsSet()) {
    if (!avrisp_timeout_ticks) {
      return false;
    }
    bp_delay_ms(1);
  }
  usb_device_state = DEFAULT_STATE;
  EnableUsbGlobalInterrupt();
  return true;
}

static bool avrisp_usb_switch_to_mkii(void) {
  avrisp_usb_detach();
  avrisp_usb_personality_active = true;
  avrisp_ep_out = NULL;
  avrisp_ep_in = NULL;
  usb_init(avrisp_device_descriptor, avrisp_config_descriptor,
           avrisp_string_descriptors, AVRISP_NUM_STRINGS);
  return avrisp_usb_attach();
}

static void avrisp_usb_switch_to_cdc(void) {
  for (;;) {
    avrisp_usb_detach();
    avrisp_usb_personality_active = false;
    usb_init(cdc_device_descriptor, cdc_config_descriptor, cdc_str_descs,
              AVRISP_CDC_NUM_STRINGS);
    initCDC();
    if (!avrisp_usb_attach()) {
      continue;
    }

    /* Keep BBIO away from unarmed descriptors. A failed Windows restart
       or configuration gets a fresh detach/attach, not a permanent wait. */
    avrisp_activity_ticks = AVRISP_IDLE_TICKS;
    while (usb_device_state < CONFIGURED_STATE) {
      if (!avrisp_activity_ticks) {
        break;
      }
    }
    if (usb_device_state >= CONFIGURED_STATE) {
      return;
    }
  }
}

static void avrisp_usb_run(void) {
  /* Do not wait for CDC IN: the caller may already have closed its COM port. */
  if (!avrisp_usb_switch_to_mkii()) {
    goto restore_cdc;
  }

  /* Wait for the host to configure the mkII personality. */
  while ((usb_device_state < CONFIGURED_STATE) && avrisp_activity_ticks) {
  }

  while ((usb_device_state >= CONFIGURED_STATE) && avrisp_activity_ticks) {
    if (!avrisp_ep_out || !avrisp_ep_in) {
      break;
    }
    if (avrisp_ep_out->BDSTAT & UOWN) {
      continue;
    }

    /* Host activity: rearm the idle watchdog and start the command one. */
    avrisp_timeout_ticks = AVRISP_TIMEOUT_TICKS;

    if (!avrisp_usb_read_request()) {
      break;
    }
    if (avrisp_request_length == 0) {
      continue;
    }

    avrisp_activity_ticks = AVRISP_IDLE_TICKS;
    avrisp_v2_process_command();

    if (!avrisp_timeout_ticks) {
      /* The transfer stalled: leave the personality. */
      break;
    }
  }

restore_cdc:
  /* Release the target even if CDC recovery must retry for a while. */
  avrisp_xprog_shutdown();
  avrisp_isp_shutdown();
  avrisp_usb_switch_to_cdc();
}


/* -------------------------------------------------------------------------
 * Serial STK500v2 fallback (AVR068 framing).
 * ------------------------------------------------------------------------- */

#define AVRISP_SERIAL_MESSAGE_START 0x1B
#define AVRISP_SERIAL_TOKEN 0x0E
#define AVRISP_SERIAL_ANSWER_CKSUM_ERROR 0xB0
#define AVRISP_SERIAL_STATUS_CKSUM_ERROR 0xC1

/** Sequence number of the frame being processed. */
static uint8_t avrisp_serial_sequence;
static bool avrisp_serial_rx_error;

static bool avrisp_serial_poll_byte(uint8_t *byte) {
  /* getc_cdc() can block forever after an otherwise harmless ZLP. */
  return poll_getc_cdc(byte) != 0;
}

static bool avrisp_serial_write_byte(const uint8_t byte) {
  while (!getInReady() && avrisp_timeout_ticks &&
         usb_device_state >= CONFIGURED_STATE) {
  }
  if (usb_device_state >= CONFIGURED_STATE && avrisp_timeout_ticks) {
    putc_cdc(byte);
    return true;
  }
  avrisp_activity_ticks = 0;
  return false;
}

static void avrisp_serial_transmit_response(void) {
  uint16_t index;
  uint8_t checksum = AVRISP_SERIAL_MESSAGE_START ^ avrisp_serial_sequence ^
                     (uint8_t)(avrisp_response_length >> 8) ^
                     (uint8_t)avrisp_response_length ^ AVRISP_SERIAL_TOKEN;

  if (!avrisp_serial_write_byte(AVRISP_SERIAL_MESSAGE_START) ||
      !avrisp_serial_write_byte(avrisp_serial_sequence) ||
      !avrisp_serial_write_byte((uint8_t)(avrisp_response_length >> 8)) ||
      !avrisp_serial_write_byte((uint8_t)avrisp_response_length) ||
      !avrisp_serial_write_byte(AVRISP_SERIAL_TOKEN)) {
    return;
  }

  for (index = 0; index < avrisp_response_length; index++) {
    if (!avrisp_serial_write_byte(avrisp_response_buffer[index])) {
      return;
    }
    checksum ^= avrisp_response_buffer[index];
  }

  (void)avrisp_serial_write_byte(checksum);
  avrisp_response_length = 0;
}

/** Waits up to 50 ms for a frame byte. */
static bool avrisp_serial_read_frame_byte(uint8_t *byte) {
  uint8_t start = avrisp_timeout_ticks;
  while (avrisp_timeout_ticks && avrisp_activity_ticks &&
         (uint8_t)(start - avrisp_timeout_ticks) < 5) {
    if (avrisp_serial_poll_byte(byte)) {
      return true;
    }
    if (avrisp_serial_rx_error) {
      return false;
    }
    bp_delay_ms(1);
  }

  return false;
}

static void avrisp_serial_run(void) {
  uint8_t byte;
  uint8_t sequence;
  uint8_t size_high;
  uint8_t size_low;
  uint8_t token;
  uint8_t checksum;
  uint16_t index;
  uint16_t length;

  for (;;) {
    if (!avrisp_activity_ticks) {
      break;
    }

    avrisp_serial_rx_error = false;
    if (!avrisp_serial_poll_byte(&byte)) {
      bp_delay_ms(1);
      continue;
    }

    avrisp_activity_ticks = AVRISP_IDLE_TICKS;

    if (byte != AVRISP_SERIAL_MESSAGE_START) {
      continue;
    }

    avrisp_timeout_ticks = AVRISP_TIMEOUT_TICKS;

    if (!avrisp_serial_read_frame_byte(&sequence) ||
        !avrisp_serial_read_frame_byte(&size_high) ||
        !avrisp_serial_read_frame_byte(&size_low) ||
        !avrisp_serial_read_frame_byte(&token)) {
      continue;
    }

    if (token != AVRISP_SERIAL_TOKEN) {
      continue;
    }

    length = ((uint16_t)size_high << 8) | size_low;

    if (length == 0 || length > AVRISP_BUFFER_SIZE) {
      /* Malformed or oversized frame: drain it and resync. */
      uint16_t drain = length;

      while (drain-- && avrisp_serial_read_frame_byte(&byte)) {
      }

      (void)avrisp_serial_read_frame_byte(&byte);
      continue;
    }

    checksum = AVRISP_SERIAL_MESSAGE_START ^ sequence ^ size_high ^ size_low ^
               token;

    for (index = 0; index < length; index++) {
      if (!avrisp_serial_read_frame_byte(&byte)) {
        break;
      }

      avrisp_command_buffer[index] = byte;
      checksum ^= byte;
    }

    if (index < length) {
      continue;
    }

    if (!avrisp_serial_read_frame_byte(&byte)) {
      continue;
    }

    if (byte != checksum) {
      /* AVR068 checksum error reply. */
      avrisp_response_length = 0;
      avrisp_response_buffer[avrisp_response_length++] =
          AVRISP_SERIAL_ANSWER_CKSUM_ERROR;
      avrisp_response_buffer[avrisp_response_length++] =
          AVRISP_SERIAL_STATUS_CKSUM_ERROR;
      avrisp_serial_sequence = sequence;
      avrisp_serial_transmit_response();
      continue;
    }

    /* Dispatch the frame; leftover bytes are dropped with the frame. */
    avrisp_serial_sequence = sequence;
    avrisp_request_length = length;
    avrisp_request_position = 0;
    avrisp_timeout_ticks = AVRISP_TIMEOUT_TICKS;

    avrisp_v2_process_command();
  }
}

/* -------------------------------------------------------------------------
 * Engine-facing transport dispatch.
 * ------------------------------------------------------------------------- */

bool avrisp_is_usb(void) {
  return avrisp_transport == AVRISP_TRANSPORT_USB;
}

uint8_t avrisp_read_byte(void) {
  if (avrisp_request_position < avrisp_request_length &&
      avrisp_request_position < AVRISP_BUFFER_SIZE) {
    return avrisp_command_buffer[avrisp_request_position++];
  }
  return 0xFF;
}

uint16_t avrisp_request_size(void) {
  return avrisp_request_length;
}

void avrisp_write_byte(const uint8_t value) {
  if (avrisp_response_length < AVRISP_BUFFER_SIZE) {
    avrisp_response_buffer[avrisp_response_length++] = value;
  }
}

void avrisp_response_begin(void) {
  avrisp_response_length = 0;
}

void avrisp_response_end(void) {
  /* A target timeout must still be reportable to the host. */
  avrisp_timeout_ticks = AVRISP_TIMEOUT_TICKS;
  avrisp_activity_ticks = AVRISP_IDLE_TICKS;
  avrisp_xprog_resume_clock();
  if (avrisp_transport == AVRISP_TRANSPORT_USB) {
    avrisp_usb_flush_response();
    return;
  }
  avrisp_serial_transmit_response();
}

void avrisp_discard_incoming(void) {
  avrisp_request_position = avrisp_request_length;
}

/* -------------------------------------------------------------------------
 * Mode entry.
 * ------------------------------------------------------------------------- */

void avrisp_mkii_enter(const avrisp_transport_kind_t transport) {
  uint16_t saved_nesting = INTCON1bits.NSTDIS;
  /* v4 normally disables nesting; the priority-6 idle clock must be able
     to preempt USB servicing to satisfy PDI's inactivity limit. */
  INTCON1bits.NSTDIS = 0;
  avrisp_transport = transport;
  avrisp_response_length = 0;
  avrisp_timeout_ticks = 0;
  avrisp_activity_ticks = AVRISP_IDLE_TICKS;


  avrisp_v2_initialise();
  avrisp_isp_initialise();
  avrisp_xprog_initialise();
  avrisp_timer_start();

  if (transport == AVRISP_TRANSPORT_USB) {
    avrisp_usb_run();
  } else {
    avrisp_serial_run();
  }

  avrisp_xprog_shutdown();
  avrisp_isp_shutdown();
  avrisp_timer_stop();
  INTCON1bits.NSTDIS = saved_nesting;
}

#endif /* BP_ENABLE_AVRISP_MKII_SUPPORT */
