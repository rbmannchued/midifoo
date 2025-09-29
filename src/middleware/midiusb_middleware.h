#ifndef MIDIUSB_MIDDLEWARE_H
#define MIDIUSB_MIDDLEWARE_H

#include <stdint.h>

void midiusb_send_noteOn(uint8_t note, uint8_t velocity, uint8_t channel);
void midiusb_send_noteOff(uint8_t note, uint8_t velocity, uint8_t channel);
void midiusb_send_cc(uint8_t control, uint8_t value, uint8_t channel);

#endif
