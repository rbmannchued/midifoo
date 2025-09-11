#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"

#include "midiusb.h"

/* --- Ações associadas a cada botão --- */
void action_pressed_button1(void) {
    gpio_clear(GPIOC, GPIO13);
    usb_send_noteOn(usbd_dev);
}
void action_released_button1(void){
    gpio_set(GPIOC, GPIO13);
    usb_send_noteOff(usbd_dev);
}
void action_button2(void) {
    gpio_clear(GPIOC, GPIO13);
}

/* --- Estrutura de botão --- */
typedef void (*button_cb_t)(void);

typedef struct {
    uint16_t pin;
    button_cb_t onPressed;
    button_cb_t onReleased;
    
} Button;

/* --- Tabela de botões --- */
static Button buttons[] = {
    { GPIO1, action_pressed_button1, action_released_button1},
    { GPIO2, action_button2, action_released_button1}
};
static const uint8_t BUTTON_NUM = sizeof(buttons) / sizeof(buttons[0]);

void led_setup(void) {
    rcc_periph_clock_enable(RCC_GPIOC);
    gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13);
    gpio_set(GPIOC, GPIO13);
}

void buttonPollTask(void *args){
    (void) args;
    rcc_periph_clock_enable(RCC_GPIOB);


    for (uint8_t i = 0; i < BUTTON_NUM; i++) {
        gpio_mode_setup(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, buttons[i].pin);
    }


    bool button_state[BUTTON_NUM];
    for (uint8_t i = 0; i < BUTTON_NUM; i++) {
        // active-low: pressionado quando gpio == 0
        button_state[i] = (gpio_get(GPIOB, buttons[i].pin) == 1);
    }

    while (1) {
        for (uint8_t i = 0; i < BUTTON_NUM; i++) {
            bool pressed = (gpio_get(GPIOB, buttons[i].pin) == 0);

            if (pressed != button_state[i]) {
                // debounce simples: espera 20ms e verifica novamente
                vTaskDelay(pdMS_TO_TICKS(20));
                pressed = (gpio_get(GPIOB, buttons[i].pin) == 0);

                if (pressed != button_state[i]) {
                    button_state[i] = pressed;

                    if (pressed && buttons[i].onPressed) {
                        buttons[i].onPressed(); 
                    }else if (!pressed && buttons[i].onReleased) {
			buttons[i].onReleased();  
		    }
		  
                }
            }
        }
        // pausa pequena para não ocupar 100% CPU
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

int main(void) {
    rcc_clock_setup_pll(&rcc_hse_25mhz_3v3[RCC_CLOCK_3V3_84MHZ]);

    led_setup();
    usbMidiInit();
    
    xTaskCreate(buttonPollTask, "buttonTask", 512, NULL, 2, NULL);
    vTaskStartScheduler();

    while (1);
}

