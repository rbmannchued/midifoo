#include "buttons_hal.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

static uint16_t *button_pins;
static uint8_t button_count;

void buttons_hal_init(uint16_t *pins, uint8_t count) {
    button_pins = pins;
    button_count = count;

    rcc_periph_clock_enable(RCC_GPIOB); // assumindo GPIOB
    for (uint8_t i = 0; i < button_count; i++) {
        gpio_mode_setup(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, button_pins[i]);
    }
}

bool buttons_hal_read(uint8_t index) {
    if (index >= button_count) return false;
    return (gpio_get(GPIOB, button_pins[index]) == 0);
}
