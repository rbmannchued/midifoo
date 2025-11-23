#ifndef BATTERY_SERVICE_H
#define BATTERY_SERVICE_H

#include <stdint.h>

void battery_service_init(void);
void battery_service_task(void *args);
uint8_t battery_service_voltage_to_percent(float volts);
float battery_service_adc_to_voltage(uint16_t raw);
uint8_t battery_service_get_percent();
void battery_service_update_display();

#endif
