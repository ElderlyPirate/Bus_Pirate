#ifndef CDC_H
#define CDC_H
#include <stdint.h>
void cdc_init(void);
void cdc_configure(void);
void cdc_reset(void);
uint8_t cdc_setup(const uint8_t *setup);
uint8_t cdc_line_data(const uint8_t *data, uint8_t length);
uint8_t cdc_get(uint8_t *byte);
uint8_t cdc_send(const uint8_t *data, uint8_t length);
uint8_t cdc_tx_ready(void);
uint8_t cdc_session_changed(void);
#endif
