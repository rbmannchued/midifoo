#include "hc595_middleware.h"
#include "hc595_hal.h"

static uint8_t current_state = 0x00;

void hc595_init(void){
    hc595_hal_init();
}
void hc595_toggle_pin(uint8_t pin) {
    
    if (pin > 7) return;
    
    current_state ^= (1 << pin);
    
    hc595_hal_byte_write(current_state);
}

void hc595_set_pin(uint8_t pin, uint8_t state) {

    if (pin > 7) return;
    
    if (state) {

        current_state |= (1 << pin);
    } else {

        current_state &= ~(1 << pin);
    }

    hc595_hal_byte_write(current_state);
}

void hc595_write_byte(uint8_t byte) {
    current_state = byte;
    hc595_hal_byte_write(current_state);
}
