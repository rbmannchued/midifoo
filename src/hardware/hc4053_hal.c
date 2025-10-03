#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include "hc4053_hal.h"

#define SER_PIN GPIO12
#define RCLK_PIN GPIO13
#define RCLR_PIN GPIO14

#define PORT_PINS GPIOB
#define RCC_PORT_PINS RCC_GPIOB

void hc4053_hal_init(void){
    rcc_periph_clock_enable(RCC_PORT_PINS);
    gpio_mode_setup(PORT_PINS, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, SER_PIN | RCLR_PIN | RCLK_PIN);

}

void hc4053_hal_byte_write(uint8_t byte){
    for (int i = 7; i >= 0; i--) {
        // Define SER
        if ((byte >> i) & 0x01)
            gpio_set(PORT_PINS, SER_PIN);
        else
            gpio_clear(PORT_PINS, SER_PIN);
	
        // Pulse on clock (SRCLK)
        gpio_clear(PORT_PINS, RCLR_PIN);
        gpio_set(PORT_PINS, RCLR_PIN);
    }

    // Pulse on  latch (RCLK)
    gpio_clear(PORT_PINS, RCLK_PIN);
    gpio_set(PORT_PINS, RCLK_PIN);
}

