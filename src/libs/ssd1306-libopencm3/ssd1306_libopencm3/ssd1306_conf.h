/**
 * Private configuration file for the SSD1306 library.
 */

#ifndef __SSD1306_CONF_H__
#define __SSD1306_CONF_H__

// Choose a microcontroller family
//#define STM32F1
#define STM32F4


// Choose a bus
#define SSD1306_USE_I2C
//#define SSD1306_USE_SPI

// I2C Configuration
#define SSD1306_I2C_PORT        I2C1
#define SSD1306_I2C_ADDR        0x3C

/*
    spi1 = 
    cs - a4
    dc - a3
    res - a6
    mosi - a7
    clk - a5
*/

/* // SPI Configuration */
/* #define SSD1306_SPI_PORT        SPI1 */
/* #define SSD1306_CS_Port         GPIOA */
/* #define SSD1306_CS_Pin          GPIO4 */
/* #define SSD1306_DC_Port         GPIOA */
/* #define SSD1306_DC_Pin          GPIO3 */
/* #define SSD1306_Reset_Port      GPIOA */
/* #define SSD1306_Reset_Pin       GPIO6 */

/* // not used on library code but useful for organization */
/* #define SSD1306_Mosi_Port       GPIOA    */
/* #define SSD1306_Mosi_Pin        GPIO7 */
/* #define SSD1306_Clk_Port        GPIOA */
/* #define SSD1306_Clk_Pin         GPIO5 */

// Mirror the screen if needed
// #define SSD1306_MIRROR_VERT
// #define SSD1306_MIRROR_HORIZ

// Set inverse color if needed
// # define SSD1306_INVERSE_COLOR

// Include only needed fonts
#define SSD1306_INCLUDE_FONT_6x8
#define SSD1306_INCLUDE_FONT_7x10
#define SSD1306_INCLUDE_FONT_11x18
#define SSD1306_INCLUDE_FONT_16x26

#define SSD1306_INCLUDE_FONT_16x24

#define SSD1306_INCLUDE_FONT_16x15

// The width of the screen can be set using this
// define. The default value is 128.
// #define SSD1306_WIDTH           64

// If your screen horizontal axis does not start
// in column 0 you can use this define to
// adjust the horizontal offset
// #define SSD1306_X_OFFSET

// The height can be changed as well if necessary.
// It can be 32, 64 or 128. The default value is 64.
// #define SSD1306_HEIGHT          64

#endif /* __SSD1306_CONF_H__ */
