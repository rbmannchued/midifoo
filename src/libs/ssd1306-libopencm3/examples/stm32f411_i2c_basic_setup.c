
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include "ssd1306.h"
#include "ssd1306_fonts.h"

void i2c_setup(void) {
    /* enable clock for GPIOB and I2C1 */
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_I2C1);

    

    /* configure pins PB6 (SCL) and PB7 (SDA) as Alternate Function */

    gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO6|GPIO7);
    gpio_set_af(GPIOB, GPIO_AF4, GPIO6|GPIO7);
    gpio_set_output_options(GPIOB, GPIO_OTYPE_OD, GPIO_OSPEED_2MHZ, GPIO6 | GPIO7);
    
    /* reset and config I2C */


    i2c_peripheral_disable(SSD1306_I2C_PORT);

    i2c_set_clock_frequency(SSD1306_I2C_PORT, I2C_CR2_FREQ_42MHZ);
    i2c_set_fast_mode(SSD1306_I2C_PORT); /* fast mode */
    i2c_set_ccr(SSD1306_I2C_PORT, 35); /* configure CCR */
    i2c_set_trise(SSD1306_I2C_PORT, 42);

    i2c_peripheral_enable(SSD1306_I2C_PORT);
}

int main(void){
    rcc_clock_setup_pll(&rcc_hse_25mhz_3v3[RCC_CLOCK_3V3_84MHZ]);
	
    i2c_setup();
	

    ssd1306_Init(); //send commands to configure ssd1306

    ssd1306_SetCursor(35,10); // set cursor x =   and y = 10
    ssd1306_WriteString("Hello ", Font_11x18,White); // writes using font
    ssd1306_Line(5,32,123,32,White); //draw line
	
	
    ssd1306_UpdateScreen(); // do not forgot to update screen!
	

    
    	
    for (;;);
    return 0;


}
