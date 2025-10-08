#include "pots_service.h"
#include "adc_hal.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/adc.h>

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

void pots_service_init() {
    adc_hal_init(&pots_adc_cfg);
    adc_hal_start();

    for (int i = 0; i < NUM_POTS; i++) {
        uint16_t raw = adc_hal_read_channel(pots_adc_cfg.channel[i]);
        current_values[i] = (uint8_t)((raw * 127) / 4095);
        previous_values[i] = current_values[i];
    }
}

uint8_t pots_service_read_midiValue(uint8_t pot) {
    if (pot >= NUM_POTS) return 0;

    uint16_t adc_value = adc_hal_read_channel(pot);
    uint8_t mapped_value = (uint8_t)((adc_value * 127) / 4095);

    previous_values[pot] = current_values[pot];
    current_values[pot] = mapped_value;

    return mapped_value;
}

bool pots_service_has_changed(uint8_t  pot) {
    if (pot >= NUM_POTS) return false;
    return (current_values[pot] != previous_values[pot]);
}


uint16_t pots_service_read_adc(uint8_t channel) {
    return adc_hal_read_channel(channel);
}
