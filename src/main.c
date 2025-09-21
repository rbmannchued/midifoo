#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include <stdbool.h>
#include "FreeRTOS.h"

#include "task.h"

#include "midiusb.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "tuner.h"

#define MIDI_CHANNEL 1
bool tunerModeActive;
int8_t noteOffset = 0;
uint8_t noteValues[] = {64, 74, 60, 67};
/* -- handles rtos --- */
TaskHandle_t xHandleButtonPoll = NULL;
TaskHandle_t xHandleOneButton = NULL;
/* --- Estrutura de botão --- */

typedef struct {
    uint8_t note_index;       // Índice da nota no array
    uint8_t* note_offset_ptr; // Ponteiro para a variável de offset
    bool activated;
} ButtonContext;

typedef void (*button_cb_t)(void *ctx);

typedef struct {
    uint16_t pin;
    button_cb_t onPressed;
    button_cb_t onReleased;
    button_cb_t onLongPress;
    void* ctx;
    
} Button;

ButtonContext note_contexts[] = {
    {0, &noteOffset, false}, // Botão 0 controla nota 0
    {1, &noteOffset, false}, // Botão 1 controla nota 1
    {2, &noteOffset, false}, // Botão 2 controla nota 2
    {3, &noteOffset, false}  // Botão 3 controla nota 3
};

void display_updateRoutine() {
    char buffer[8];
    snprintf(buffer, sizeof(buffer), "A%d", noteOffset);
    
    ssd1306_Fill(Black);  // limpa tela
    ssd1306_SetCursor(50, 20);
    ssd1306_WriteString(buffer, Font_16x26, White);
    ssd1306_UpdateScreen();
}

/* --- Ações associadas a cada botão --- */
void action_pressed_sendNote(void *ctx) {
    ButtonContext *context = (ButtonContext*)ctx;
    context->activated = !context->activated;
    uint8_t velocity = context->activated ? 127 : 0;
    gpio_clear(GPIOC, GPIO13);
    usb_send_cc(usbd_dev, noteValues[context->note_index] + *(context->note_offset_ptr), velocity,  MIDI_CHANNEL);
}
void action_released_sendNote(void *ctx){
    /* ButtonContext *context = (ButtonContext*)ctx; */
    /* gpio_set(GPIOC, GPIO13); */
    /* usb_send_noteOff(usbd_dev, noteValues[context->note_index] + *(context->note_offset_ptr)); */
}

void action_increase_offset(void* ctx) {
    int8_t* offset = (int8_t*)ctx;
    if (*offset < 4) {
        (*offset)++;
    }else{
	(*offset = 0);
    }
    display_updateRoutine();
}

void action_decrease_offset(void* ctx) {
    int8_t* offset = (int8_t*)ctx;
    if (*offset > 0) {
        (*offset)--;
    }else{
	(*offset = 4);
    }
    display_updateRoutine();
}
void action_toggle_tunerMode(void *ctx) {
    (void)ctx;
    tunerModeActive = !tunerModeActive;
    openToTune(tunerModeActive);

    if (tunerModeActive) {
        audio_start();
	vTaskPrioritySet(xHandleButtonPoll, 2);
        vTaskResume(xHandleAudioAcq);
        vTaskResume(xHandleFFTProc);
        
    } else {
	display_updateRoutine();
        audio_stop();
	vTaskPrioritySet(xHandleButtonPoll, 3);
	vTaskSuspend(xHandleAudioAcq);
        vTaskSuspend(xHandleFFTProc);
    }
}

/* --- Tabela de botões --- */
static Button buttons[] = {
    { GPIO1, action_pressed_sendNote, action_released_sendNote, NULL, &note_contexts[0] },
    { GPIO2, action_pressed_sendNote, action_released_sendNote, NULL, &note_contexts[1] },
    { GPIO3, action_pressed_sendNote, action_released_sendNote, NULL, &note_contexts[2] },
    { GPIO4, action_pressed_sendNote, action_released_sendNote, NULL, &note_contexts[3] },
    { GPIO5, action_decrease_offset, NULL, action_toggle_tunerMode, &noteOffset },
    { GPIO8, action_increase_offset, NULL, action_toggle_tunerMode, &noteOffset }
};

static const uint8_t BUTTON_NUM = sizeof(buttons) / sizeof(buttons[0]);

void led_setup(void) {
    rcc_periph_clock_enable(RCC_GPIOC);
    gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13);
    gpio_set(GPIOC, GPIO13);
}

void button_poll_Task(void *args){
    (void) args;
    rcc_periph_clock_enable(RCC_GPIOB);

    for (uint8_t i = 0; i < BUTTON_NUM; i++) {
        gpio_mode_setup(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, buttons[i].pin);
    }

    bool button_state[BUTTON_NUM];
    TickType_t press_time[BUTTON_NUM];

    for (uint8_t i = 0; i < BUTTON_NUM; i++) {
        button_state[i] = (gpio_get(GPIOB, buttons[i].pin) == 1);
        press_time[i] = 0;
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

                    if (pressed) {
                        // --- Se tuner está ativo, qualquer clique desativa ---
                        if (tunerModeActive) {
                            action_toggle_tunerMode(NULL); // força sair do tuner
                        } else {
                            // comportamento normal
                            press_time[i] = xTaskGetTickCount();
                            if (buttons[i].onPressed) {
                                buttons[i].onPressed(buttons[i].ctx);
                            }
                        }

                    } else if (!tunerModeActive) {
                        // Só executa release/long press se NÃO estiver no tuner
                        TickType_t held_ticks = xTaskGetTickCount() - press_time[i];
                        uint32_t held_ms = held_ticks * portTICK_PERIOD_MS;

                        if (held_ms >= 1000) {
                            if (buttons[i].onLongPress) {
                                buttons[i].onLongPress(buttons[i].ctx);
                            }
                        } else {
                            if (buttons[i].onReleased) {
                                buttons[i].onReleased(buttons[i].ctx);
                            }
                        }
                    }
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void display_init_Task(){
    /* enable clock for GPIOB and I2C1 */
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_I2C1);


    /* configure pins PB6 (SCL) and PB7 (SDA) as Alternate Function */
    gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO6|GPIO7);
    gpio_set_af(GPIOB, GPIO_AF4, GPIO6|GPIO7);
    gpio_set_output_options(GPIOB, GPIO_OTYPE_OD, GPIO_OSPEED_2MHZ, GPIO6 | GPIO7);
    
    /* reset and config I2C */


    i2c_peripheral_disable(SSD1306_I2C_PORT);
    i2c_set_speed(SSD1306_I2C_PORT, i2c_speed_fm_400k, rcc_apb1_frequency/1e6);

    i2c_peripheral_enable(SSD1306_I2C_PORT);


    ssd1306_Init();
    //fixing led bugs on display
    ssd1306_Line(130, 1, 130, 64, Black);
    ssd1306_Line(129, 1, 129, 64, Black);
    
    display_updateRoutine();

    vTaskDelete(NULL);
}

void openToTune(bool option){
    if(option){
     	gpio_set(GPIOA, GPIO6);
	gpio_clear(GPIOC, GPIO13);
    }else{
	gpio_clear(GPIOA, GPIO6);
	gpio_set(GPIOC, GPIO13);
    }
}


int main(void) {
    rcc_clock_setup_pll(&rcc_hse_25mhz_3v3[RCC_CLOCK_3V3_84MHZ]);
    
    led_setup();
    usbMidi_init();
    tuner_init();
    openToTune(true);
    xTaskCreate(display_init_Task, "displayTask", 512, NULL, 5, NULL);
    xTaskCreate(button_poll_Task, "buttonTask", 512, NULL, 3, &xHandleButtonPoll); 
    xTaskCreate(vTaskAudioAcquisition, "AudioAcq", 512, NULL, 3, &xHandleAudioAcq);
    xTaskCreate(vTaskFFTProcessing, "FFTProc", 1024, NULL, 4, &xHandleFFTProc);

    vTaskSuspend(xHandleAudioAcq);
    vTaskSuspend(xHandleFFTProc);
    audio_stop();
    vTaskStartScheduler();
    while (1);
}

