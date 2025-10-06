#include "adc_hal.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/f4/nvic.h>

static const adc_hal_config_t *adc_cfg_ptr = NULL;

void adc_hal_init(const adc_hal_config_t *cfg) {
    adc_power_off(cfg->adc);

    rcc_periph_clock_enable(cfg->rcc_gpio);
    rcc_periph_clock_enable(cfg->rcc_adc);
    for(uint8_t i = 0; i < MAX_ADC; i++){
	gpio_mode_setup(cfg->gpio_port, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, cfg->gpio_pins[i]);
    }


    /* if old config exists */
    if (adc_cfg_ptr) {
        if (adc_cfg_ptr->irq != 0) {
            nvic_disable_irq(adc_cfg_ptr->irq);
            adc_disable_eoc_interrupt(adc_cfg_ptr->adc);
        }
        adc_disable_external_trigger_regular(adc_cfg_ptr->adc);
    }

    adc_set_clk_prescale(ADC_CCR_ADCPRE_BY8);
    adc_set_resolution(cfg->adc, ADC_CR1_RES_12BIT);
    adc_set_single_conversion_mode(cfg->adc);
    adc_set_regular_sequence(cfg->adc, 1, &cfg->channel);


    if (cfg->irq != 0) {
        adc_enable_eoc_interrupt(cfg->adc);
        nvic_enable_irq(cfg->irq);
    }

    if (cfg->ext_trigger != 0 && cfg->ext_edge != 0) {
        adc_enable_external_trigger_regular(cfg->adc, cfg->ext_trigger, cfg->ext_edge);
    }


    adc_cfg_ptr = cfg;
}

void adc_hal_start() {
    adc_power_on(adc_cfg_ptr->adc);
    //adc_start_conversion_regular(cfg->adc);
	
}

void adc_hal_stop() {
    adc_power_off(adc_cfg_ptr->adc);
}


uint16_t adc_hal_read(){
    if(adc_cfg_ptr->mode != ADC_MODE_INTERRUPT){
	adc_set_regular_sequence(adc_cfg_ptr->adc, 1, adc_cfg_ptr->channel);
	adc_start_start_conversion_regular(adc_cfg_ptr->adc);
	while (!adc_eoc(ADC1));
	return adc_read_regular(ADC1);
    }else{
	return adc_read_regular(adc_cfg_ptr->adc);
    }

}
uint16_t adc_hal_read_channel(uint8_t channel){
    if(adc_cfg_ptr->mode != ADC_MODE_INTERRUPT){
	adc_set_regular_sequence(adc_cfg_ptr->adc, 1, &channel);
	adc_start_conversion_regular(adc_cfg_ptr->adc);
	while (!adc_eoc(adc_cfg_ptr->adc));
	return adc_read_regular(adc_cfg_ptr->adc);
    }

}

void adc_isr(void) {
    if (adc_cfg_ptr && adc_eoc(adc_cfg_ptr->adc)) {
        uint16_t value = adc_read_regular(adc_cfg_ptr->adc);
        if (adc_cfg_ptr->isr_callback) {
            adc_cfg_ptr->isr_callback(value);
        }
    }
}
