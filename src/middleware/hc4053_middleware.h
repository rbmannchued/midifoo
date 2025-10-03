#ifndef HC4053_MIDDLEWARE_H
#define HC4053_MIDDLEWARE_H

#include <stdint.h>

void hc4053_toggle_pin(uint8_t pin);

void hc4053_set_pin(uint8_t pin, uint8_t state);

void hc4053_write_byte(uint8_t byte);

void hc4053_init(void);
#endif
