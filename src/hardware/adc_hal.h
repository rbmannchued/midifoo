#ifndef ADC_HAL_H
#define ADC_HAL_H

#include <stdint.h>
#include <stdlib.h>
#include <libopencm3/stm32/adc.h>

typedef void (*isr_callback_t)(uint16_t sample);

typedef struct {
    uint32_t adc;        
    uint32_t gpio_port;
    uint16_t gpio_pin;
    uint8_t channel;
    uint32_t rcc_gpio;
    uint32_t rcc_adc;
    uint8_t irq;
    isr_callback_t isr_callback;
    uint32_t ext_trigger;
    uint32_t ext_edge; 
    
} adc_hal_config_t;

void adc_hal_init(const adc_hal_config_t *cfg);
void adc_hal_start(const adc_hal_config_t *cfg);
void adc_hal_stop(const adc_hal_config_t *cfg);
uint16_t adc_hal_read(const adc_hal_config_t *cfg);

#endif
