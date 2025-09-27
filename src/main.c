#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include <stdbool.h>
#include "FreeRTOS.h"

#include "task.h"

#include "midiusb.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "tuner.h"
#include "buttons_service.h"

#define MIDI_CHANNEL 1

/* -- handles rtos --- */
TaskHandle_t xHandleButtonPoll = NULL;
TaskHandle_t xHandleOneButton = NULL;


bool tunerModeActivated = false;
int8_t noteOffset = 0;
uint8_t noteValues[] = {64, 74, 60, 67};

typedef struct {
    uint8_t note_index;
    int8_t *note_offset_ptr;
    bool activated;
} ButtonContext;

static ButtonContext note_contexts[] = {
    {0, &noteOffset, false},
    {1, &noteOffset, false},
    {2, &noteOffset, false},
    {3, &noteOffset, false}
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
void action_sendCC(void *ctx) {
    ButtonContext *context = (ButtonContext*)ctx;
    context->activated = !context->activated;
    uint8_t velocity = context->activated ? 127 : 0;
    gpio_clear(GPIOC, GPIO13);
    usb_send_cc(usbd_dev, noteValues[context->note_index] + *(context->note_offset_ptr), velocity,  MIDI_CHANNEL);
}

void action_increase_offset(void* ctx) {
    if (tunerModeActivated) return;
    int8_t* offset = (int8_t*)ctx;
    if (*offset < 4) {
        (*offset)++;
    }else{
	(*offset = 0);
    }
    display_updateRoutine();
}

void action_decrease_offset(void* ctx) {
    if (tunerModeActivated) return;
    int8_t* offset = (int8_t*)ctx;
    if (*offset > 0) {
        (*offset)--;
    }else{
	(*offset = 4);
    }
    display_updateRoutine();
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

void action_toggle_tunerMode(void *ctx) {
    (void)ctx;
    tunerModeActivated = !tunerModeActivated;
    openToTune(tunerModeActivated);

    if (tunerModeActivated) {
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

static void button_handler(ButtonEventType event, ButtonEvent *ctx) {
    // Se tuner está ativo, qualquer botão desativa o modo
    if (tunerModeActivated) {
        action_toggle_tunerMode(NULL);
        return;
    }

    switch (ctx->id) {
        case 0:
        case 1:
        case 2:
        case 3: // botões de nota
            if (event == BUTTON_EVENT_RELEASED) {
                action_sendCC(ctx->user_ctx);
            }
            break;

        case 4: // offset down
            if (event == BUTTON_EVENT_RELEASED) {
                action_decrease_offset(ctx->user_ctx);
            } else if (event == BUTTON_EVENT_LONGPRESS) {
                action_toggle_tunerMode(NULL);
            }
            break;

        case 5: // offset up
            if (event == BUTTON_EVENT_RELEASED) {
                action_increase_offset(ctx->user_ctx);
            } else if (event == BUTTON_EVENT_LONGPRESS) {
                action_toggle_tunerMode(NULL);
            }
            break;
    }
}

static Button buttons[] = {
    { GPIO1, button_handler, &note_contexts[0] },
    { GPIO2, button_handler, &note_contexts[1] },
    { GPIO3, button_handler, &note_contexts[2] },
    { GPIO4, button_handler, &note_contexts[3] },
    { GPIO5, button_handler, &noteOffset },
    { GPIO8, button_handler, &noteOffset }
};


void led_setup(void) {
    rcc_periph_clock_enable(RCC_GPIOC);
    gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13);
    gpio_set(GPIOC, GPIO13);
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




int main(void) {
    rcc_clock_setup_pll(&rcc_hse_25mhz_3v3[RCC_CLOCK_3V3_84MHZ]);
    
    led_setup();
    buttons_service_init(buttons, sizeof(buttons)/sizeof(buttons[0]));
    usbMidi_init();
    tuner_init();
    openToTune(true);
    xTaskCreate(display_init_Task, "displayTask", 512, NULL, 5, NULL);
    xTaskCreate(buttons_poll_task, "buttonTask", 512, NULL, 3, &xHandleButtonPoll); 
    xTaskCreate(vTaskAudioAcquisition, "AudioAcq", 512, NULL, 3, &xHandleAudioAcq);
    xTaskCreate(vTaskFFTProcessing, "FFTProc", 1024, NULL, 4, &xHandleFFTProc);

    vTaskSuspend(xHandleAudioAcq);
    vTaskSuspend(xHandleFFTProc);
    audio_stop();
    vTaskStartScheduler();
    while (1);
}

