#include "battery_service.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/adc.h>

#include "adc_hal.h"
#include "display_service.h"
#include "FreeRTOS.h"
#include "task.h"


#define ADC_MAX 4095.0f
#define VREF    3.3f

static const adc_hal_config_t batt_adc_cfg = {
    .adc       = ADC1,
    .gpio_port = GPIOA,
    .gpio_pins = { GPIO7, 0, 0, 0 },
    .channel   = { 7, 0, 0, 0 },
    .rcc_gpio  = RCC_GPIOA,
    .rcc_adc   = RCC_ADC1,
    .irq       = 0,
    .isr_callback = NULL,
    .ext_trigger = 0,
    .ext_edge = 0,
    .mode = ADC_MODE_POLLING,
};

float battery_service_adc_to_voltage(uint16_t raw){
    float v_adc = (raw / ADC_MAX) * VREF;
    return v_adc * 2.0f; 
}

uint8_t battery_service_voltage_to_percent(float volts){
    if (volts >= 4.05f) return 100;
    if (volts >= 3.85f) return 75;
    if (volts >= 3.70f) return 50;
    if (volts >= 3.55f) return 25;
    return 0;
}

uint8_t battery_service_get_percent(){
    (void)adc_hal_read_channel(7);    // leitura descatada para evitar
    vTaskDelay(pdMS_TO_TICKS(1));
    uint16_t raw = adc_hal_read_channel(7);
    float volts = battery_service_adc_to_voltage(raw);
    uint8_t percent = battery_service_voltage_to_percent(volts);
	
    return percent;
}

void battery_service_update_display(){
    uint8_t newPercent = battery_service_get_percent();
    display_service_showBatteryIcon(newPercent);
}


void battery_service_task(void *args){
    (void)args;

    while (1) {
	battery_service_update_display();
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void battery_service_init(void){
    adc_hal_init(&batt_adc_cfg);
    adc_hal_start();
    /* xTaskCreate(battery_task, "batteryTask", 256, NULL, 2, NULL); */
}
