#include "pots_service.h"
#include "adc_hal.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/adc.h>
#include <stdlib.h>

#define POT_CHANGE_THRESHOLD 2
#define POT_SMOOTH_FACTOR 0.2f

static const adc_hal_config_t pots_adc_cfg = {
	.adc = ADC1,
	.gpio_port = GPIOA,
	.gpio_pins = {GPIO4, GPIO5},
	.channel = {4,5},
	.rcc_gpio = RCC_GPIOA,
	.rcc_adc = RCC_ADC1,
	.irq = 0,
	.isr_callback = NULL,
	.ext_trigger = 0,
	.ext_edge = 0,
	.mode = ADC_MODE_POLLING
};

static uint8_t current_values[NUM_POTS] = {0};
static uint8_t previous_values[NUM_POTS] = {0};
static float smoothed_values[NUM_POTS] = {0};

void pots_service_init() {
    adc_hal_init(&pots_adc_cfg);
    adc_hal_start();

    for (int i = 0; i < NUM_POTS; i++) {
        uint16_t raw = adc_hal_read_channel(pots_adc_cfg.channel[i]);
        current_values[i] = (uint8_t)((raw * 127) / 4095);
        previous_values[i] = current_values[i];
    }
}

uint8_t pots_service_get_midiValue(uint8_t pot) {
    uint16_t adc_value = adc_hal_read_channel(pots_adc_cfg.channel[pot]);
    uint8_t mapped_value = (adc_value * 127) / 4095;

    smoothed_values[pot] = (POT_SMOOTH_FACTOR * mapped_value) +
	((1.0f - POT_SMOOTH_FACTOR) * smoothed_values[pot]);

    previous_values[pot] = current_values[pot];
    current_values[pot] = (uint8_t)smoothed_values[pot];

    return current_values[pot];
}
void pots_service_stop(){
    adc_hal_stop();
}

bool pots_service_has_changed(uint8_t  pot) {
    int8_t diff = current_values[pot] - previous_values[pot];
    return abs(diff) >= POT_CHANGE_THRESHOLD;
}

uint16_t pots_service_read_adc(uint8_t channel) {
    return adc_hal_read_channel(channel);
}
