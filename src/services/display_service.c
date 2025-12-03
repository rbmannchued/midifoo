#include "display_service.h"
#include "i2c_hal.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include <math.h>
#include <stdio.h>


//bitmaps for battery icon 20x12
const unsigned char epd_bitmap_batt_100 []  = {
	0xff, 0xff, 0xe0, 0x80, 0x00, 0x20, 0xbb, 0xbb, 0xa0, 0xbb, 0xbb, 0xb0, 0xbb, 0xbb, 0xb0, 0xbb, 
	0xbb, 0xb0, 0xbb, 0xbb, 0xb0, 0xbb, 0xbb, 0xb0, 0xbb, 0xbb, 0xb0, 0xbb, 0xbb, 0xa0, 0x80, 0x00, 
	0x20, 0xff, 0xff, 0xe0
};

const unsigned char epd_bitmap_batt_75 [] = {
	0xff, 0xff, 0xe0, 0x80, 0x00, 0x20, 0xbb, 0xb8, 0x20, 0xbb, 0xb8, 0x30, 0xbb, 0xb8, 0x30, 0xbb, 
	0xb8, 0x30, 0xbb, 0xb8, 0x30, 0xbb, 0xb8, 0x30, 0xbb, 0xb8, 0x30, 0xbb, 0xb8, 0x20, 0x80, 0x00, 
	0x20, 0xff, 0xff, 0xe0
};

const unsigned char epd_bitmap_batt_50 [] = {
	0xff, 0xff, 0xe0, 0x80, 0x00, 0x20, 0xbb, 0x80, 0x20, 0xbb, 0x80, 0x30, 0xbb, 0x80, 0x30, 0xbb, 
	0x80, 0x30, 0xbb, 0x80, 0x30, 0xbb, 0x80, 0x30, 0xbb, 0x80, 0x30, 0xbb, 0x80, 0x20, 0x80, 0x00, 
	0x20, 0xff, 0xff, 0xe0
};

const unsigned char epd_bitmap_batt_25 [] = {
	0xff, 0xff, 0xe0, 0x80, 0x00, 0x20, 0xb8, 0x00, 0x20, 0xb8, 0x00, 0x30, 0xb8, 0x00, 0x30, 0xb8, 
	0x00, 0x30, 0xb8, 0x00, 0x30, 0xb8, 0x00, 0x30, 0xb8, 0x00, 0x30, 0xb8, 0x00, 0x20, 0x80, 0x00, 
	0x20, 0xff, 0xff, 0xe0
};

const unsigned char epd_bitmap_batt_0 [] = {
	0xff, 0xff, 0xe0, 0x80, 0x00, 0x20, 0x80, 0x00, 0x20, 0x80, 0x00, 0x30, 0x80, 0x00, 0x30, 0x80, 
	0x00, 0x30, 0x80, 0x00, 0x30, 0x80, 0x00, 0x30, 0x80, 0x00, 0x30, 0x80, 0x00, 0x20, 0x80, 0x00, 
	0x20, 0xff, 0xff, 0xe0
};

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
    ssd1306_FillRectangle(10, 40, 90, 11, Black); // clean the region first
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
void display_service_showBatteryIcon(uint8_t level) {
    ssd1306_FillRectangle(108, 0, 127, 11, Black); // clean the region first
    switch(level) {



    case 25:
        ssd1306_DrawBitmap(128-20, 0, epd_bitmap_batt_25, 20, 12, White);
        break;

    case 50:
        ssd1306_DrawBitmap(128-20, 0, epd_bitmap_batt_50, 20, 12, White);
        break;

    case 75:
        ssd1306_DrawBitmap(128-20, 0, epd_bitmap_batt_75, 20, 12, White);
        break;

    case 100:
        ssd1306_DrawBitmap(128-20, 0, epd_bitmap_batt_100, 20, 12, White);
        break;

    default:
        ssd1306_DrawBitmap(128-20, 0, epd_bitmap_batt_0, 20, 12, White);
        break;
    }

    ssd1306_UpdateScreen();
}
