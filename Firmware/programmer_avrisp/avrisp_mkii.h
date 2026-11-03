/**
 * @file avrisp_mkii.h
 *
 * @brief AVRISP-MKII personality - transport layer and shared state.
 *
 * The protocol engine is a port of LUFA's AVRISP-MKII project
 * (Copyright (C) Dean Camera, 2021; MIT-style licence, see
 * Documentation/avrisp-mkii-license.txt) rewritten for the PIC24-based
 * Bus Pirate.  The transport layer presents the engine with a byte stream
 * on either the mkII USB personality (bulk endpoint 2) or the framed
 * STK500v2 serial fallback (AVR068).
 */

#ifndef BP_AVRISP_MKII_H
#define BP_AVRISP_MKII_H

#include "base.h"

#ifdef BP_ENABLE_AVRISP_MKII_SUPPORT

/* Use latches for writes: PORT read/modify/write can corrupt adjacent pins. */
#define AVRISP_CLK_LAT  LATDbits.LATD2
#define AVRISP_MOSI_LAT LATDbits.LATD1
#define AVRISP_CS_LAT   LATDbits.LATD4
#define AVRISP_AUX_LAT  LATDbits.LATD5

/**
 * Maximum number of bytes held for a command request payload or a
 * response.  The largest request is a 256 byte page write plus its
 * header (266 bytes); the largest response is a 256 byte memory read
 * (259 bytes).
 */
#define AVRISP_BUFFER_SIZE 288

/** Command timeout, in 10 ms timer ticks (1 second, LUFA parity). */
#define AVRISP_TIMEOUT_TICKS 100

/** Idle time with no host activity before the mode ends, in 10 ms ticks. */
#define AVRISP_IDLE_TICKS 500

/** Transports supported by the personality. */
typedef enum {
  AVRISP_TRANSPORT_USB = 0,
  AVRISP_TRANSPORT_SERIAL
} avrisp_transport_kind_t;

/** Request payload buffer (also used for serial frame reassembly). */
extern uint8_t avrisp_command_buffer[AVRISP_BUFFER_SIZE];

/** Response buffer, transmitted when the response completes. */
extern uint8_t avrisp_response_buffer[AVRISP_BUFFER_SIZE];

/** Command watchdog, decremented every 10 ms by the mode timer. */
extern volatile uint8_t avrisp_timeout_ticks;


/** True while the mkII USB personality is enumerated (see dp_usb/cdc.c). */
extern volatile bool avrisp_usb_personality_active;

/** Arms the bulk endpoint 2 pair for the mkII personality. */
void avrisp_usb_configure_endpoints(void);


/**
 * Enters the AVRISP-MKII mode; returns when the host has been idle long
 * enough, or when a transfer stalls.
 */
void avrisp_mkii_enter(avrisp_transport_kind_t transport);

/** Returns true when the active transport is the mkII USB personality. */
bool avrisp_is_usb(void);

/** Reads the next request byte (0xFF when the watchdog expires). */
uint8_t avrisp_read_byte(void);

/** Complete request length, including command bytes, before dispatch. */
uint16_t avrisp_request_size(void);

/** Appends a byte to the response being built. */
void avrisp_write_byte(uint8_t value);

/** Starts a response (the complete request, including its ZLP, is buffered). */
void avrisp_response_begin(void);

/** Sends the completed response, terminating the transfer. */
void avrisp_response_end(void);

/** Discards any remaining bytes of the current request transfer. */
void avrisp_discard_incoming(void);

#endif /* BP_ENABLE_AVRISP_MKII_SUPPORT */

#endif /* !BP_AVRISP_MKII_H */
