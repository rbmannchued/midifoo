#ifndef TUNER_SERVICE_H
#define TUNER_SERVICE_H

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "dsp.h"       
#include "notes.h"
#include "adc_hal.h"
#include "timer_hal.h"
#include "display_service.h"

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/f4/nvic.h>
#include <libopencm3/cm3/cortex.h>


extern TaskHandle_t xHandleAudioAcq;
extern TaskHandle_t xHandleFFTProc;

//--

void tuner_service_init();
void audio_start();
void audio_stop();
void tuner_audioAcq_task(void *pvParameters);
void tuner_processing_task(void *pvParameters);
static void tuner_adc_isr_callback(uint16_t sample);
void tuner_service_stop();
#endif
