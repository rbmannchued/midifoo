#ifndef HC4053_HAL_H
#define HC4053_HAL_H

#include <stdint.h>

void hc4053_hal_init(void);

/* Write a byte to the shiftregister less signifcant bit first */
void hc4053_hal_byte_write(uint8_t byte);

#endif
