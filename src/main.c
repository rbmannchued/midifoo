#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"

#include "midiusb.h"

#define midiChannel 1
int8_t noteOffset = 0;
uint8_t noteValues[] = {64, 74, 60, 67};

/* --- Estrutura de botão --- */

typedef struct {
    uint8_t note_index;       // Índice da nota no array
    uint8_t* note_offset_ptr; // Ponteiro para a variável de offset
} ButtonContext;

typedef void (*button_cb_t)(void *ctx);

typedef struct {
    uint16_t pin;
    button_cb_t onPressed;
    button_cb_t onReleased;
    void* ctx;
    
} Button;

ButtonContext note_contexts[] = {
    {0, &noteOffset}, // Botão 0 controla nota 0
    {1, &noteOffset}, // Botão 1 controla nota 1
    {2, &noteOffset}, // Botão 2 controla nota 2
    {3, &noteOffset}  // Botão 3 controla nota 3
};
/* --- Ações associadas a cada botão --- */
void action_pressed_sendNote(void *ctx) {
    ButtonContext *context = (ButtonContext*)ctx;
    gpio_clear(GPIOC, GPIO13);
    usb_send_cc(usbd_dev, noteValues[context->note_index] + *(context->note_offset_ptr), midiChannel);
}
void action_released_sendNote(void *ctx){
    /* ButtonContext *context = (ButtonContext*)ctx; */
    /* gpio_set(GPIOC, GPIO13); */
    /* usb_send_noteOff(usbd_dev, noteValues[context->note_index] + *(context->note_offset_ptr)); */
}

void action_increase_offset(void* ctx) {
    uint8_t* offset = (uint8_t*)ctx;
    if (*offset < 127 - 60) {
        (*offset)++;
    }
}

void action_decrease_offset(void* ctx) {
    uint8_t* offset = (uint8_t*)ctx;
    if (*offset > 0) {
        (*offset)--;
    }
}


/* --- Tabela de botões --- */
static Button buttons[] = {
    { GPIO1, action_pressed_sendNote, action_released_sendNote, &note_contexts[0] },
    { GPIO2, action_pressed_sendNote, action_released_sendNote, &note_contexts[1] },
    { GPIO3, action_pressed_sendNote, action_released_sendNote, &note_contexts[2] },
    { GPIO4, action_pressed_sendNote, action_released_sendNote, &note_contexts[3] },
    { GPIO5, action_decrease_offset, NULL, &noteOffset },
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
                        buttons[i].onPressed(buttons[i].ctx); 
                    }else if (!pressed && buttons[i].onReleased) {
			buttons[i].onReleased(buttons[i].ctx);  
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

