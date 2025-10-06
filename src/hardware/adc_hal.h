#ifndef ADC_HAL_H
#define ADC_HAL_H

#include <stdint.h>
#include <stdlib.h>
#include <libopencm3/stm32/adc.h>

#define MAX_ADC 4
typedef void (*isr_callback_t)(uint16_t sample);

typedef enum {
    ADC_MODE_POLLING,
    ADC_MODE_INTERRUPT
} adc_mode_t;


typedef struct {
    uint32_t adc;        
    uint32_t gpio_port;
    uint16_t gpio_pins[MAX_ADC];
    uint8_t channel[MAX_ADC];
    uint32_t rcc_gpio;
    uint32_t rcc_adc;
    uint8_t irq;
    isr_callback_t isr_callback;
    uint32_t ext_trigger;
    uint32_t ext_edge;
    adc_mode_t mode;
    
} adc_hal_config_t;

void adc_hal_init(const adc_hal_config_t *cfg);
uint16_t adc_hal_read_channel(uint8_t channel);
void adc_hal_start();
void adc_hal_stop();
uint16_t adc_hal_read();

#endif
