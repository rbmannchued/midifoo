#include "midibt_middleware.h"
#include "usart_hal.h"
#include "hc05_hal.h"

void midibt_init(){
    usart_hal_init(115200);
    hc05_init();
}

void midibt_send_cc(uint8_t control, uint8_t value, uint8_t channel){
    if(hc05_is_connected()){
	uint8_t buf[4] = {
	    0x0B,
	    (uint8_t)(0xB0 | (channel & 0x0F)),
	    control,
	    value
	};
	for(int i = 0; i < 4; i++){
            usart_send_value(buf[i]);
        }
    }
}

