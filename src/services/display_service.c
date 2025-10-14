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
    ssd1306_Fill(Black);

    char buf[8];
    snprintf(buf, sizeof(buf), "%s%d", noteName, octave);
    ssd1306_SetCursor(8, 10);
    ssd1306_WriteString(buf, Font_16x26, White);

    snprintf(buf, sizeof(buf), "%+d¢", noteDiff);
    ssd1306_SetCursor(90, 10);
    ssd1306_WriteString(buf, Font_11x18, White);

    const int centerX = 64;
    const int barTop = 50;
    const int barBottom = 63;
    ssd1306_Line(centerX, barTop, centerX, barBottom, White);

    int pointerOffset = (int)(noteDiff * 0.8f);
    if (pointerOffset > 40) pointerOffset = 40;
    if (pointerOffset < -40) pointerOffset = -40;
    int pointerX = centerX + pointerOffset;

    ssd1306_Line(pointerX, barBottom, pointerX, barTop - 5, White);

    ssd1306_Line(centerX - 40, barTop, centerX - 40, barBottom, White);
    ssd1306_Line(centerX + 40, barTop, centerX + 40, barBottom, White);

    ssd1306_UpdateScreen();
}

void display_service_showNoSignal(void) {
    ssd1306_Fill(Black);
    ssd1306_SetCursor(8, 10);
    ssd1306_WriteString("...", Font_16x26, White);
    ssd1306_UpdateScreen();
}
