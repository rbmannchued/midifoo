#ifndef DISPLAY_SERVICE_H
#define DISPLAY_SERVICE_H

#include <stdint.h>
/* init i2c and display procedures */
void display_service_init();

/* shows A0, A1... */
void display_service_showNoteBank(int8_t offset);

/* shows note and meter */
void display_service_showTunerInfo(int noteDiff, const char *noteName, int octave);

void display_service_showNoSignal(void);

void display_service_showBatteryIcon(uint8_t level);

void display_service_eraseScreen();
  
#endif
