#ifndef BUTTONS_HAL_H
#define BUTTONS_HAL_H

#include <stdint.h>
#include <stdbool.h>

void buttons_hal_init(uint16_t *pins, uint8_t count);
bool buttons_hal_read(uint8_t index);

#endif
