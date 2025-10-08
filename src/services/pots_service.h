#ifndef POTS_SERVICE_H
#define POTS_SERVICE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "adc_hal.h"

#define NUM_POTS 2

static uint8_t current_values[NUM_POTS];
static uint8_t previous_values[NUM_POTS];
void pots_service_init(void);
uint8_t pots_service_read_midiValue(uint8_t pot);
bool pots_service_has_changed(uint8_t  pot);
uint16_t pots_service_read_adc(uint8_t channel);
void pots_poll_task(void *pv);
#endif
