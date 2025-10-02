#ifndef USART_HAL_H
#define USART_HAL_H

#include <stdint.h>

void usart_hal_init(uint32_t baudrate);

void usart_hal_send_string(const char *str);

char usart_hal_receive_char(void);

void usart_send_value(uint8_t value);

#endif
