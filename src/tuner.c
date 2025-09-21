#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/f4/nvic.h>
#include <libopencm3/cm3/cortex.h>

#include "ssd1306.h"
#include "ssd1306_fonts.h"

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


void display_result(int noteDiff, double frequency, int noteIndex){
    char frequencyStr[15];
    char octaveStr[10];
    int octave = get_noteOctave(noteIndex);
    ssd1306_SetCursor(0, 10);
    snprintf(frequencyStr,sizeof(frequencyStr),"%.2f hz \n",frequency);
    snprintf(octaveStr, sizeof(octaveStr), "%d", octave);
	  
	     
    ssd1306_Fill(Black);
    ssd1306_Line(64,0,(64+(128*noteDiff/100)),0, White);
    ssd1306_Line(64,1,(64+(128*noteDiff/100)),1, White);
    ssd1306_Line(64,2,(64+(128*noteDiff/100)),2, White);
    ssd1306_Line(64,3,(64+(128*noteDiff/100)),3, White);
    ssd1306_Line(64,4,(64+(128*noteDiff/100)),4, White);
    ssd1306_Line(64,5,(64+(128*noteDiff/100)),5, White);
    ssd1306_Line(64,6,(64+(128*noteDiff/100)),6, White);
    ssd1306_Line(64,7,(64+(128*noteDiff/100)),7, White);
    ssd1306_Line(64,8,(64+(128*noteDiff/100)),8, White);
    ssd1306_Line(64,9,(64+(128*noteDiff/100)),9, White);
    ssd1306_SetCursor(30,20);
    ssd1306_WriteString("  ",Font_11x18, White);


    ssd1306_WriteString(noteNames[noteIndex % 12], Font_16x26, White);
    ssd1306_SetCursor(85,26);
    ssd1306_WriteString(octaveStr, Font_11x18,White);
    ssd1306_SetCursor(40,50);
    ssd1306_WriteString(frequencyStr,Font_7x10, White);
    ssd1306_UpdateScreen();
     
}

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

    // Inicializa periféricos de aquisição
    adc_init();
    timer_init();

    // Inicializa objetos FreeRTOS antes de criar tasks
    xBufferReadySemaphore = xSemaphoreCreateBinary();
    xAudioQueue = xQueueCreate(2, sizeof(volatile uint16_t *));

    // Inicializa o motor DSP (CMSIS-DSP)
    dsp_init();
}

void audio_start(void) {
    adc_power_off(ADC1);
    adc_power_on(ADC1);

    // Reinicia timer / trigger
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
            // note: processing_buffer é volatile uint16_t *
            xQueueSend(xAudioQueue, &processing_buffer, portMAX_DELAY);
        }
    }
}

void vTaskFFTProcessing(void *pvParameters) {
    volatile uint16_t *vbuffer_ptr = NULL;

    // últimos valores para manter na tela caso não detecte nada
    static int lastNoteIndex = 0;
    static int lastNoteDiff = 0;
    static double lastFrequency = 0.0;

    for (;;) {
        if (xQueueReceive(xAudioQueue, &vbuffer_ptr, portMAX_DELAY) == pdTRUE) {
            // DSP não usa volatile; convertemos o ponteiro antes de passar
            uint16_t *buffer_ptr = (uint16_t *)vbuffer_ptr;

            // chama pipeline DSP (retorna frequência em Hz)
            float frequency = dsp_process(buffer_ptr);

            // interpretar resultado no nível da aplicação
            int noteIndex = get_closestNoteIndex((double)frequency);

            if (frequency == 0.0f || noteIndex == 60 || noteIndex == 0) {
                // mantém último resultado na tela
                display_result(lastNoteDiff, lastFrequency, lastNoteIndex);
            } else {
                double noteDiff_d = get_noteDiff((double)frequency, noteIndex);
                int noteDiff = (int)round(noteDiff_d);
                display_result(noteDiff, (double)frequency, noteIndex);

                lastNoteDiff = noteDiff;
                lastFrequency = (double)frequency;
                lastNoteIndex = noteIndex;
            }

            // Pode ajustar delay conforme necessidade (simula a Task do firmware)
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}
