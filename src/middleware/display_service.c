#include "display_service.h"
#include "i2c_hal.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include <math.h>
#include <stdio.h>

void display_service_init(){
    i2c_hal_init();
    ssd1306_Init();
    
    //fix led bugs on display
    ssd1306_Line(130, 1, 130, 64, Black);
    ssd1306_Line(129, 1, 129, 64, Black);
    
    ssd1306_UpdateScreen();
}

void display_service_showNoteBank(int8_t offset) {
    char buf[8];
    snprintf(buf, sizeof(buf), "A%d", offset);
    ssd1306_Fill(Black);
    ssd1306_SetCursor(50, 20);
    ssd1306_WriteString(buf, Font_16x26, White);
    ssd1306_UpdateScreen();
}

void display_service_showTunerInfo(int noteDiff, const char *noteName, int octave) {
    char octaveStr[10];

    ssd1306_Fill(Black);

    for (int y = 0; y < 10; y++) {
        ssd1306_Line(64, y, (64 + (128 * noteDiff / 100)), y, White);
    }


    ssd1306_SetCursor(30, 20);
    ssd1306_WriteString("  ", Font_11x18, White);
    ssd1306_WriteString(noteName, Font_16x26, White);


    snprintf(octaveStr, sizeof(octaveStr), "%d", octave);
    ssd1306_SetCursor(85, 26);
    ssd1306_WriteString(octaveStr, Font_11x18, White);

    ssd1306_UpdateScreen();
}
