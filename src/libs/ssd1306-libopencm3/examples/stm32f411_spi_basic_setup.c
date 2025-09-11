
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include "ssd1306.h"
#include "ssd1306_fonts.h"


void init(){					
  //init pins and spi
	//clock setup for stm32f1x

  rcc_clock_setup_hse_3v3(&rcc_hse_25mhz_3v3[RCC_CLOCK_3V3_84MHZ]);
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_SPI1);

    //set reset pin as output
    
  
    gpio_mode_setup(SSD1306_Reset_Port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, SSD1306_Reset_Pin);

    gpio_mode_setup(SSD1306_DC_Port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, SSD1306_DC_Pin);

    gpio_mode_setup(SSD1306_Mosi_Port, GPIO_MODE_AF, GPIO_PUPD_NONE, SSD1306_Mosi_Pin);
    gpio_set_af(SSD1306_Mosi_Port, GPIO_AF5, SSD1306_Mosi_Pin);

    gpio_mode_setup(SSD1306_Clk_Port, GPIO_MODE_AF, GPIO_PUPD_NONE, SSD1306_Clk_Pin);
    gpio_set_af(SSD1306_Clk_Port, GPIO_AF5, SSD1306_Clk_Pin);

    gpio_mode_setup(SSD1306_CS_Port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, SSD1306_CS_Pin);

    //SPI INIT 
  spi_init_master(
		SSD1306_SPI_PORT,SPI_CR1_BAUDRATE_FPCLK_DIV_256,
        SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
		SPI_CR1_CPHA_CLK_TRANSITION_1,
	    SPI_CR1_DFF_8BIT,
	    SPI_CR1_MSBFIRST
	);
	spi_disable_software_slave_management(SSD1306_SPI_PORT);
	spi_enable_ss_output(SSD1306_SPI_PORT);


}


int main(){

	init();

	ssd1306_Init(); //send commands to configure ssd1306

	ssd1306_SetCursor(35,10); // set cursor x =  35 and y = 10
	ssd1306_WriteString("Hello", Font_11x18,White); // writes using font
	ssd1306_Line(5,32,123,32,White); //draw line

	ssd1306_UpdateScreen(); // do not forgot to update screen!

    
    	
    for (;;);
	return 0;


}
