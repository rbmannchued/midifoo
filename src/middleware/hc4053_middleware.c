#include "hc4053_middleware.h"
#include "hc4053_hal.h"

static uint8_t current_state = 0x00;

void hc4053_init(void){
    hc4053_hal_init();
}
void hc4053_toggle_pin(uint8_t pin) {
    
    if (pin > 7) return;
    
    current_state ^= (1 << pin);
    
    hc4053_hal_byte_write(current_state);
}

void hc4053_set_pin(uint8_t pin, uint8_t state) {

    if (pin > 7) return;
    
    if (state) {

        current_state |= (1 << pin);
    } else {

        current_state &= ~(1 << pin);
    }

    hc4053_hal_byte_write(current_state);
}

void hc4053_write_byte(uint8_t byte) {
    current_state = byte;
    hc4053_hal_byte_write(current_state);
}
