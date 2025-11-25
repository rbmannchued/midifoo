#include "led_service.h"
#include "hc595_middleware.h"

static bool led_states[NUM_BANKS][NUM_BUTTONS] = {0};
static uint8_t current_bank = 0;
static bool tuner_mode = false;

void led_service_init(void) {
    hc595_hal_init();
    /* Start all leds off */
    hc595_write_byte(0b00000000);
    
    for (int bank = 0; bank < NUM_BANKS; bank++) {
        for (int led = 0; led < NUM_BUTTONS; led++) {
            led_states[bank][led] = false;
        }
    }
}

void led_service_set_led(uint8_t bank, uint8_t led_index, bool state) {
    if (bank >= NUM_BANKS || led_index >= NUM_BUTTONS) return;
    
    led_states[bank][led_index] = state;
   
    if (bank == current_bank && !tuner_mode) {
        if (state) {
            hc595_set_pin(led_index, 1);
        } else {
            hc595_set_pin(led_index, 0);
        }
    }
}

void led_service_toggle_led(uint8_t bank, uint8_t led_index) {
    if (bank >= NUM_BANKS || led_index >= NUM_BUTTONS) return;
    
    led_states[bank][led_index] = !led_states[bank][led_index];
    
    if (bank == current_bank && !tuner_mode) {
        if (led_states[bank][led_index]) {
            hc595_set_pin(led_index, 1);
        } else {
            hc595_set_pin(led_index, 0);
        }
    }
}

void led_service_update_bank(uint8_t bank) {
    if (bank >= NUM_BANKS) return;
    
    current_bank = bank;
    
    if (!tuner_mode) {
        for (int i = 0; i < NUM_BUTTONS; i++) {
            if (led_states[bank][i]) {
               hc595_set_pin(i, 1);
            } else {
                hc595_set_pin(i, 0);
            }
        }
    }
}

void led_service_clear_all(void) {

    hc595_write_byte(0x00);
}

void led_service_set_tuner_mode(bool enabled) {
    tuner_mode = enabled;
    
    if (tuner_mode) {
        led_service_clear_all();
    } else {
        led_service_update_bank(current_bank);
    }
}

bool led_service_get_led_state(uint8_t bank, uint8_t led_index) {
    if (bank >= NUM_BANKS || led_index >= NUM_BUTTONS) return false;
    return led_states[bank][led_index];
}
