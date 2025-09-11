
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include "ssd1306.h"
#include "ssd1306_fonts.h"


void init(){					//init pins and spi
	//clock setup for stm32f1x
	rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_SPI1);

	//set reset pin as output
    gpio_set_mode(SSD1306_Reset_Port,
		GPIO_MODE_OUTPUT_2_MHZ,
		GPIO_CNF_OUTPUT_PUSHPULL,
		SSD1306_Reset_Pin);
	//set dc pin as output
    gpio_set_mode(SSD1306_DC_Port,
		GPIO_MODE_OUTPUT_2_MHZ,
		GPIO_CNF_OUTPUT_PUSHPULL,
		SSD1306_DC_Pin);
    //set mosi pin as output altfn
	gpio_set_mode(SSD1306_Mosi_Port,GPIO_MODE_OUTPUT_50_MHZ,
        GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
        SSD1306_Mosi_Pin
	);

	//set clk pin as output altfn
	gpio_set_mode(SSD1306_Clk_Port,GPIO_MODE_OUTPUT_50_MHZ,
        GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
        SSD1306_Clk_Pin
	);
    // set cs pin as output altfn
	gpio_set_mode(SSD1306_CS_Port,GPIO_MODE_OUTPUT_50_MHZ,
        GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
        SSD1306_CS_Pin
	);

	// init Spi master using arguments
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