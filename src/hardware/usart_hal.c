#include "usart_hal.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>


#define USART_PORT USART2
#define USART_RCC_PORT RCC_USART2
#define USART_TX_PIN GPIO2
#define USART_RX_PIN GPIO3
#define USART_PORT_PIN GPIOA
#define USART_RCC_PORT_PIN RCC_GPIOA

/* CODE FOR USART_PORT */
void usart_hal_init(uint32_t baudrate) {

    rcc_periph_clock_enable(USART_RCC_PORT_PIN);
    rcc_periph_clock_enable(USART_RCC_PORT);


    gpio_mode_setup(USART_PORT_PIN, GPIO_MODE_AF, GPIO_PUPD_NONE, USART_TX_PIN | USART_RX_PIN);
    gpio_set_af(USART_PORT_PIN, GPIO_AF7, USART_TX_PIN | USART_RX_PIN); // AF7 = USART_PORT

    usart_set_baudrate(USART_PORT, baudrate); // HC-05 padrão é 9600 bps
    usart_set_databits(USART_PORT, 8);
    usart_set_stopbits(USART_PORT, USART_STOPBITS_1);
    usart_set_mode(USART_PORT, USART_MODE_TX_RX);
    usart_set_parity(USART_PORT, USART_PARITY_NONE);
    usart_set_flow_control(USART_PORT, USART_FLOWCONTROL_NONE);

    usart_enable(USART_PORT);
}

void usart_hal_send_string(const char *str) {
    while (*str) {
	usart_send_blocking(USART_PORT, *str++);
    }
}

char usart_hal_receive_char(void) {
    return usart_recv_blocking(USART_PORT);
}

void usart_send_value(uint8_t value){
    usart_send_blocking(USART_PORT, value);
}


