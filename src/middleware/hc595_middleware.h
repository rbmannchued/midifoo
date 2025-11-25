#ifndef HC595_MIDDLEWARE_H
#define HC595_MIDDLEWARE_H

#include <stdint.h>

void hc595_toggle_pin(uint8_t pin);

void hc595_set_pin(uint8_t pin, uint8_t state);

void hc595_write_byte(uint8_t byte);

void hc595_init(void);
#endif
