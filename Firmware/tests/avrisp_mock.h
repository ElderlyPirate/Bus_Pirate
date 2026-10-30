#ifndef AVRISP_NATIVE_MOCK_H
#define AVRISP_NATIVE_MOCK_H
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define BUSPIRATEV4
#define BP_ENABLE_AVRISP_MKII_SUPPORT
#define ROMPTR
#define ON 1
#define OFF 0
#define HIGH 1
#define LOW 0
#define INPUT 1
#define OUTPUT 0
#define OC5_IO 22
#define SDO1_IO 7
#define SCK1OUT_IO 8
#define BP_MISO_RPIN 22
#define BP_ADC_PROBE 1
#define _SPI1CON1_PPRE_POSITION 0
#define _SPI1CON1_MSTEN_POSITION 5
#define _SPI1CON1_CKE_POSITION 8
static struct { unsigned LATD1, LATD2, LATD4, LATD5; } LATDbits;
static unsigned BP_MISO, BP_MOSI_DIR, BP_CLK_DIR, BP_CS_DIR, BP_AUX0_DIR;
static unsigned BP_MISO_DIR, BP_MOSI_RPOUT, BP_CLK_RPOUT, BP_CS_RPOUT;
static unsigned BP_MISO_RPOUT, BP_AUX_RPOUT;
static unsigned BP_MOSI_ODC, BP_CLK_ODC, BP_MISO_ODC, BP_CS_ODC, BP_AUX_ODC;
static unsigned T2CON, TMR2, PR2, OC5CON, OC5CON2, OC5TMR, OC5R, OC5RS;
static unsigned T3CON, TMR3, PR3, T4CON, TMR4, PR4;
static unsigned SPI1BUF, SPI1CON1, SPI1CON2, SPI1STAT;
static unsigned AD1CON1, AD1CON2, AD1CON3, AD1CSSL;
static struct { unsigned SPIEN; } SPI1STATbits;
static struct { unsigned SDI1R; } RPINR20bits;
static struct { unsigned TON; } T2CONbits, T4CONbits;
static struct { unsigned TON, TCKPS; } T3CONbits;
static struct { unsigned T3IF, T2IF, SPI1IF; } IFS0bits;
static struct { unsigned T3IE; } IEC0bits;
static struct { unsigned T3IP; } IPC2bits;
static struct { unsigned T4IF; } IFS1bits;
static struct { unsigned T4IE; } IEC1bits;
static struct { unsigned T4IP; } IPC6bits;
static struct { unsigned CN52PUE; } CNPU4bits;
static struct { unsigned USBEN; } U1CONbits;
static struct { unsigned NSTDIS; } INTCON1bits;
typedef struct {
  unsigned char BDCNT;
  volatile unsigned char BDSTAT;
  unsigned char *BDADDR;
} BDentry;
#define USB_EP0_BUFFER_SIZE 8
#define USB_EP_INOUT 0x1e
#define USB_DIR_IN 1
#define USB_DIR_OUT 0
#define USB_PP_EVEN 0
#define USB_CALC_BD(ep, dir, pp) ((ep)*2+(dir))
#define UOWN 0x80
#define DTS 0x40
#define DTSEN 0x08
#define DEFAULT_STATE 0x04
#define ATTACHED_STATE 0x01
#define CONFIGURED_STATE 0x20
static unsigned USB_UEP1, USB_UEP2;
static BDentry usb_bdt[6];
extern volatile uint8_t usb_device_state;
static void mock_usb_in(void);
static void mock_usb_out(void);
static void mock_spi(void);
static void mock_bit(bool drive, bool bit, bool sample);
static void mock_usb_configure(void);
static void mock_usb_run_poll(void);
static void mock_response_append(void);
static bool mock_enumerating;
static unsigned mock_configuration_polls, mock_sof_registrations;
static bool mock_stuck_se0;
static unsigned mock_attach_attempts, mock_fail_configurations;
static void bp_delay_us(unsigned us);
static void bp_delay_ms(unsigned ms);
static bool user_serial_ready_to_read(void);
static uint8_t user_serial_read_byte(void);
static void user_serial_transmit_character(uint8_t b);
static void bp_adc_pin_setup(void) {}
static void bp_enable_adc(void) {}
static void bp_disable_adc(void) {}
static uint16_t bp_read_adc(unsigned channel) { (void)channel; return 512; }
static unsigned poll_getc_cdc(uint8_t *byte) {
  if (!user_serial_ready_to_read()) return 0;
  *byte = user_serial_read_byte();
  return 1;
}
static unsigned getInReady(void);
static void putc_cdc(uint8_t byte) { user_serial_transmit_character(byte); }
static void usb_unset_in_handler(unsigned ep) { (void)ep; }
static void usb_unset_out_handler(unsigned ep) { (void)ep; }
static void DisableGlobalUsbInterrupt(void) {}
static void EnableUsb(void) { mock_attach_attempts++; }
static bool SingleEndedZeroIsSet(void) { return mock_stuck_se0; }
static void EnableUsbGlobalInterrupt(void) {
  if (!mock_enumerating) {
    usb_device_state = CONFIGURED_STATE;
#ifdef BUSPIRATEV4
    extern volatile bool avrisp_usb_personality_active;
    void avrisp_usb_configure_endpoints(void);
    if (avrisp_usb_personality_active) avrisp_usb_configure_endpoints();
#endif
  }
}
static void initCDC(void) {}
static void CDCFlushOnTimeout(void) {}
static void usb_register_sof_handler(void (*f)(void)) {
  (void)f;
  mock_sof_registrations++;
}
static void usb_init(const unsigned char *a, const unsigned char *b,
                     const unsigned char *c, unsigned n) {
  (void)a; (void)b; (void)c; (void)n;
}
#endif
