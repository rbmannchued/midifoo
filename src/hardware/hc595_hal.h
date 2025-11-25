#ifndef HC595_HAL_H
#define HC595_HAL_H

#include <stdint.h>

void hc595_hal_init(void);

/* Write a byte to the shiftregister less signifcant bit first */
void hc595_hal_byte_write(uint8_t byte);

#endif
