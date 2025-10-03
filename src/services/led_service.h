#ifndef LED_SERVICE_H
#define LED_SERVICE_H

#include <stdint.h>
#include <stdbool.h>

#define NUM_BANKS 5
#define NUM_BUTTONS 4

void led_service_init(void);
void led_service_set_led(uint8_t bank, uint8_t led_index, bool state);
void led_service_toggle_led(uint8_t bank, uint8_t led_index);
void led_service_update_bank(uint8_t bank);
void led_service_clear_all(void);
bool led_service_get_led_state(uint8_t bank, uint8_t led_index);
void led_service_set_tuner_mode(bool enabled);

#endif
