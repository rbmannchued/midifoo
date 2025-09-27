#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>

void i2c_hal_init(){
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_I2C1);


    /* configure pins PB6 (SCL) and PB7 (SDA) as Alternate Function */
    gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO6|GPIO7);
    gpio_set_af(GPIOB, GPIO_AF4, GPIO6|GPIO7);
    gpio_set_output_options(GPIOB, GPIO_OTYPE_OD, GPIO_OSPEED_2MHZ, GPIO6 | GPIO7);
    
    /* reset and config I2C */


    i2c_peripheral_disable(I2C1);
    i2c_set_speed(I2C1, i2c_speed_fm_400k, rcc_apb1_frequency/1e6);

    i2c_peripheral_enable(I2C1);
}
