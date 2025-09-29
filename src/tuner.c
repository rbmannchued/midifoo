#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/f4/nvic.h>
#include <libopencm3/cm3/cortex.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "dsp.h"       
#include "notes.h"

//rtos objects
SemaphoreHandle_t xBufferReadySemaphore = NULL;
QueueHandle_t xAudioQueue = NULL;

TaskHandle_t xHandleAudioAcq = NULL;
TaskHandle_t xHandleFFTProc = NULL;

//--
volatile uint16_t adc_buffer1[FRAME_LEN];
volatile uint16_t adc_buffer2[FRAME_LEN];
volatile uint16_t *current_buffer = adc_buffer1;
volatile uint16_t *processing_buffer = adc_buffer2;
volatile int buffer_index = 0;

void adc_isr(void) {
    if (adc_eoc(ADC1)) {
        uint16_t adc_value = adc_read_regular(ADC1);

        current_buffer[buffer_index++] = adc_value;

        if (buffer_index >= FRAME_LEN) {
            buffer_index = 0;

            // Troca buffers
            volatile uint16_t *temp = current_buffer;
            current_buffer = processing_buffer;
            processing_buffer = temp;

            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xSemaphoreGiveFromISR(xBufferReadySemaphore, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
}

static void timer_init(void) {
    rcc_periph_clock_enable(RCC_TIM2);
    timer_set_prescaler(TIM2, 209);
    timer_set_period(TIM2, 99);
    timer_generate_event(TIM2, TIM_EGR_UG);
    timer_clear_flag(TIM2, TIM_EGR_UG);
    timer_set_master_mode(TIM2, TIM_CR2_MMS_UPDATE);
    timer_enable_counter(TIM2);
}

static void adc_init(void) {
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_ADC1);
    gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO1);

    adc_power_off(ADC1);
    adc_set_clk_prescale(ADC_CCR_ADCPRE_BY8);
    adc_set_resolution(ADC1, ADC_CR1_RES_12BIT);
    adc_set_single_conversion_mode(ADC1);
    uint8_t channel = 1;
    adc_set_regular_sequence(ADC1, 1, &channel);
    adc_enable_eoc_interrupt(ADC1);
    nvic_enable_irq(NVIC_ADC_IRQ);
    adc_enable_external_trigger_regular(ADC1, ADC_CR2_EXTSEL_TIM2_TRGO, ADC_CR2_EXTEN_RISING_EDGE);
    adc_power_on(ADC1);
}

void tuner_init(void) {
    cm_enable_interrupts();

    adc_init();
    timer_init();

    xBufferReadySemaphore = xSemaphoreCreateBinary();
    xAudioQueue = xQueueCreate(2, sizeof(volatile uint16_t *));

    dsp_init();
}

void audio_start(void) {
    adc_power_off(ADC1);
    adc_power_on(ADC1);

    // Reinicia timer
    timer_disable_counter(TIM2);
    timer_enable_counter(TIM2);

    adc_start_conversion_regular(ADC1);

    if (xHandleAudioAcq) vTaskResume(xHandleAudioAcq);
    if (xHandleFFTProc) vTaskResume(xHandleFFTProc);
}

void audio_stop(void) {
    timer_disable_counter(TIM2);
    adc_power_off(ADC1);

    if (xHandleAudioAcq) vTaskSuspend(xHandleAudioAcq);
    if (xHandleFFTProc) vTaskSuspend(xHandleFFTProc);

    buffer_index = 0;
}

void vTaskAudioAcquisition(void *pvParameters) {
    for (;;) {
        if (xSemaphoreTake(xBufferReadySemaphore, portMAX_DELAY) == pdTRUE) {
            // envia ponteiro do buffer pronto para a fila
            xQueueSend(xAudioQueue, &processing_buffer, portMAX_DELAY);
        }
    }
}

void vTaskFFTProcessing(void *pvParameters) {
    volatile uint16_t *vbuffer_ptr = NULL;

    static int lastNoteDiff = 0;
    static int lastNoteIndex = 0;

    for (;;) {
        if (xQueueReceive(xAudioQueue, &vbuffer_ptr, portMAX_DELAY) == pdTRUE) {

            uint16_t *buffer_ptr = (uint16_t *)vbuffer_ptr;

            float frequency = dsp_process(buffer_ptr);

            int noteIndex = get_closestNoteIndex(frequency);

            if (frequency == 0.0f || noteIndex == 60 || noteIndex == 0) {

                display_service_showTunerInfo(
                    lastNoteDiff,
                    noteNames[lastNoteIndex % 12],
                    get_noteOctave(lastNoteIndex)
                );
            } else {
                double noteDiff_d = get_noteDiff(frequency, noteIndex);
                int noteDiff = (int)round(noteDiff_d);

                display_service_showTunerInfo(
                    noteDiff,
                    noteNames[noteIndex % 12],
                    get_noteOctave(noteIndex)
                );

                lastNoteDiff = noteDiff;
                lastNoteIndex = noteIndex;
            }

            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}
