#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/cortex.h>

#include <stdbool.h>
#include "FreeRTOS.h"

#include "task.h"

#include "midiusb_middleware.h"
#include "tuner_service.h"
#include "buttons_service.h"
#include "display_service.h"

#define MIDI_CHANNEL 1

#define NUM_BANKS 5
#define NUM_BUTTONS 4

/* -- handles rtos --- */
TaskHandle_t xHandleButtonPoll = NULL;
TaskHandle_t xHandleOneButton = NULL;


bool tunerModeActivated = false;
int8_t noteOffset = 0;
uint8_t noteValues[] = {64, 74, 60, 67};

bool button_states[NUM_BANKS][NUM_BUTTONS] = {0};

typedef struct {
    uint8_t note_index;
    int8_t *note_offset_ptr;
} ButtonContext;

static ButtonContext note_contexts[] = {
    {0, &noteOffset},
    {1, &noteOffset},
    {2, &noteOffset},
    {3, &noteOffset}
};

void action_pressed(void *ctx) {
    ButtonContext *context = (ButtonContext*)ctx;
    uint8_t bank = *(context->note_offset_ptr);
    uint8_t idx = context->note_index;

    button_states[bank][idx] = !button_states[bank][idx];

    uint8_t velocity = button_states[bank][idx] ? 127 : 0;
    gpio_clear(GPIOC, GPIO13);
    midiusb_send_cc(noteValues[idx] + bank, velocity, MIDI_CHANNEL);
}

void action_increase_offset(void* ctx) {
    if (tunerModeActivated) return;
    int8_t* offset = (int8_t*)ctx;
    if (*offset < 4) {
        (*offset)++;
    }else{
	(*offset = 0);
    }
    display_service_showNoteBank(noteOffset);
}

void action_decrease_offset(void* ctx) {
    if (tunerModeActivated) return;
    int8_t* offset = (int8_t*)ctx;
    if (*offset > 0) {
        (*offset)--;
    }else{
	(*offset = 4);
    }
    display_service_showNoteBank(noteOffset);    
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
	display_service_showTunerInfo(0, " ", 0); // mostra tela tuner inicial
        
    } else {
	display_service_showNoteBank(noteOffset);
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
                action_pressed(ctx->user_ctx);
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
void display_startTask(){
    display_service_init();
    display_service_showNoteBank(noteOffset);
    vTaskDelete(NULL);
}

int main(void) {
    rcc_clock_setup_pll(&rcc_hse_25mhz_3v3[RCC_CLOCK_3V3_84MHZ]);
    
    led_setup();
    buttons_service_init(buttons, sizeof(buttons)/sizeof(buttons[0]));
    midiusb_init();
    tuner_service_init();

    cm_enable_interrupts(); // enables global interrupts needed for tuner adc
    openToTune(true);
    
    xTaskCreate(display_startTask, "displayTask", 512, NULL, 6, NULL);
    xTaskCreate(buttons_poll_task, "buttonTask", 512, NULL, 3, &xHandleButtonPoll); 
    xTaskCreate(tuner_audioAcq_task, "tunerAudioAcq", 512, NULL, 3, &xHandleAudioAcq);
    xTaskCreate(tuner_processing_task, "tunerProcessing", 1024, NULL, 4, &xHandleFFTProc);
    
    vTaskSuspend(xHandleAudioAcq);
    vTaskSuspend(xHandleFFTProc);

    audio_stop();
    vTaskStartScheduler();
    while (1);
}

