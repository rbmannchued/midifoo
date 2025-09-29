#include "midiusb_middleware.h"
#include "midiusb_hal.h"

void midiusb_send_noteOn(uint8_t note, uint8_t velocity, uint8_t channel) {
    uint8_t buf[4] = {
        0x09,            // CIN=Note On
        (uint8_t)(0x90 | (channel & 0x0F)),
        note,
        velocity
    };
    while (midiusb_hal_write(buf, sizeof(buf)) == 0);
}

void midiusb_send_noteOff(uint8_t note, uint8_t velocity, uint8_t channel) {
    uint8_t buf[4] = {
        0x08,            // CIN=Note Off
        (uint8_t)(0x80 | (channel & 0x0F)),
        note,
        velocity
    };
    while (midiusb_hal_write(buf, sizeof(buf)) == 0);
}

void midiusb_send_cc(uint8_t control, uint8_t value, uint8_t channel) {
    uint8_t buf[4] = {
        0x0B,            // CIN=Control Change
        (uint8_t)(0xB0 | (channel & 0x0F)),
        control,
        value
    };
    while (midiusb_hal_write(buf, sizeof(buf)) == 0);
}

void midiusb_init(){
    midiusb_hal_init();
}
