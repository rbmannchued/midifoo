#include "tuner_service.h"


//rtos objects
SemaphoreHandle_t xBufferReadySemaphore = NULL;
QueueHandle_t xAudioQueue = NULL;

TaskHandle_t xHandleAudioAcq = NULL;
TaskHandle_t xHandleFFTProc = NULL;

volatile uint16_t adc_buffer1[FRAME_LEN];
volatile uint16_t adc_buffer2[FRAME_LEN];
volatile uint16_t *current_buffer = adc_buffer1;
volatile uint16_t *processing_buffer = adc_buffer2;
volatile int buffer_index = 0;

static void tuner_adc_isr_callback(uint16_t sample);

// libopencm3 sṕecifics
static adc_hal_config_t tuner_adc_cfg = {
    .adc = ADC1,
    .gpio_port = GPIOA,
    .gpio_pins = GPIO1,
    .channel = 1,
    .rcc_gpio = RCC_GPIOA,
    .rcc_adc = RCC_ADC1,
    .irq = NVIC_ADC_IRQ,
    .isr_callback = tuner_adc_isr_callback,
    .ext_trigger = ADC_CR2_EXTSEL_TIM2_TRGO,
    .ext_edge = ADC_CR2_EXTEN_RISING_EDGE,
    .mode = ADC_MODE_INTERRUPT
};

static timer_hal_config_t tuner_timer_cfg = {
    .timer = TIM2,
    .rcc_timer = RCC_TIM2,
    .prescaler = 209,
    .period = 99  
};

void tuner_service_init(){
    adc_hal_init(&tuner_adc_cfg);
    timer_hal_init(&tuner_timer_cfg);

    //FreeRTOS instances semaphre and queue
    xBufferReadySemaphore = xSemaphoreCreateBinary();
    xAudioQueue = xQueueCreate(2, sizeof(volatile uint16_t *));

    dsp_init();
}

static void tuner_adc_isr_callback(uint16_t sample) {
    current_buffer[buffer_index++] = sample;

    if (buffer_index >= FRAME_LEN) {
        buffer_index = 0;

        volatile uint16_t *temp = current_buffer;
        current_buffer = processing_buffer;
        processing_buffer = temp;

        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(xBufferReadySemaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void audio_start(){
    adc_hal_stop();
    adc_hal_start();

    timer_hal_stop(&tuner_timer_cfg);
    timer_hal_start(&tuner_timer_cfg);

    if (xHandleAudioAcq) vTaskResume(xHandleAudioAcq);
    if (xHandleFFTProc) vTaskResume(xHandleFFTProc);
}


void audio_stop() {
    timer_hal_stop(&tuner_timer_cfg);
    adc_hal_stop();

    if (xHandleAudioAcq) vTaskSuspend(xHandleAudioAcq);
    if (xHandleFFTProc) vTaskSuspend(xHandleFFTProc);

    buffer_index = 0;
}


void tuner_audioAcq_task(void *pvParameters) {
    for (;;) {
        if (xSemaphoreTake(xBufferReadySemaphore, portMAX_DELAY) == pdTRUE) {

            xQueueSend(xAudioQueue, &processing_buffer, portMAX_DELAY);
        }
    }
}

void tuner_processing_task(void *pvParameters) {
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

