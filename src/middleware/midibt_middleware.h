#ifndef MIDIBT_MIDDLEWARE_H
#define MIDIBT_MIDDLEWARE_H

#include <stdint.h>

void midibt_init();

void midibt_send_cc(uint8_t control, uint8_t value, uint8_t channel);

#endif
