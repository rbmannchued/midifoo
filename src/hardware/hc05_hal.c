#include "hc05_hal.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#define STATE_RCC_PORT RCC_GPIOB
#define STATE_PORT GPIOB
#define STATE_PIN GPIO15

void hc05_init(){
    rcc_periph_clock_enable(STATE_RCC_PORT);
    gpio_mode_setup(STATE_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, STATE_PIN);

}

bool hc05_is_connected(){
    return gpio_get(STATE_PORT, STATE_PIN);
}
