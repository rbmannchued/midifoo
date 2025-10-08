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

uint8_t pots_service_read_midiValue(uint8_t channel) {

    uint16_t adc_value = adc_hal_read_channel(channel);
    uint8_t mapped_value = (adc_value * 127) / 4095;

    previous_values[channel] = current_values[channel];
    current_values[channel] = mapped_value;

    return mapped_value;
}
void pots_service_stop(){
    adc_hal_stop();
}

bool pots_service_has_changed(uint8_t  channel) {
    return (current_values[channel] != previous_values[channel]);
}

bool pots_service_read_and_check(uint8_t channel, uint8_t *out_value) {
    uint16_t adc_value = adc_hal_read_channel(channel);
    uint8_t mapped_value = (adc_value * 127) / 4095;

    bool changed = (mapped_value != current_values[channel]);

    previous_values[channel] = current_values[channel];
    current_values[channel] = mapped_value;
    *out_value = mapped_value;

    return changed;
}


uint16_t pots_service_read_adc(uint8_t channel) {
    return adc_hal_read_channel(channel);
}
