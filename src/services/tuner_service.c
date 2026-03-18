#include "tuner_service.h"
#include <string.h>

// Processa a cada HOP_SIZE novas amostras (em vez de esperar FRAME_LEN)
// Latência: HOP_SIZE / SAMPLE_RATE = 512 / 4000 = 128ms
#define HOP_SIZE 512

//rtos objects
SemaphoreHandle_t xBufferReadySemaphore = NULL;
QueueHandle_t xAudioQueue = NULL;

TaskHandle_t xHandleAudioAcq = NULL;
TaskHandle_t xHandleFFTProc = NULL;

// Ring buffer: sempre mantém as últimas FRAME_LEN amostras
static volatile uint16_t adc_ring[FRAME_LEN];
static volatile uint16_t adc_snapshot[FRAME_LEN];
static volatile int ring_write = 0;
static volatile int hop_count = 0;

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
    .prescaler = 239,
    .period    = 99
};

void tuner_service_init(){
    adc_hal_init(&tuner_adc_cfg);
    timer_hal_init(&tuner_timer_cfg);

    //FreeRTOS instances semaphre and queue
    if (!xBufferReadySemaphore) xBufferReadySemaphore = xSemaphoreCreateBinary();
    if (!xAudioQueue) xAudioQueue = xQueueCreate(2, sizeof(volatile uint16_t *));
    

    dsp_init();
}

static void tuner_adc_isr_callback(uint16_t sample) {
    adc_ring[ring_write] = sample;
    ring_write = (ring_write + 1) % FRAME_LEN;

    if (++hop_count >= HOP_SIZE) {
        hop_count = 0;
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(xBufferReadySemaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void audio_start(){
    adc_hal_start();

    timer_hal_stop(&tuner_timer_cfg);
    timer_hal_start(&tuner_timer_cfg);

    if (xHandleAudioAcq) vTaskResume(xHandleAudioAcq);
    if (xHandleFFTProc) vTaskResume(xHandleFFTProc);
}
void tuner_service_stop(){
    audio_stop();
}


void audio_stop() {
    timer_hal_stop(&tuner_timer_cfg);
    adc_hal_stop();

    if (xHandleAudioAcq) vTaskSuspend(xHandleAudioAcq);
    if (xHandleFFTProc) vTaskSuspend(xHandleFFTProc);

    ring_write = 0;
    hop_count = 0;
}


void tuner_audioAcq_task(void *pvParameters) {
    for (;;) {
        if (xSemaphoreTake(xBufferReadySemaphore, portMAX_DELAY) == pdTRUE) {
            // Lineariza o ring buffer (amostra mais antiga primeiro)
            int head = ring_write;
            int tail_len = FRAME_LEN - head;
            memcpy((void*)adc_snapshot,            (void*)(adc_ring + head), tail_len * sizeof(uint16_t));
            memcpy((void*)(adc_snapshot + tail_len), (void*)adc_ring,        head    * sizeof(uint16_t));

            volatile uint16_t *snap = adc_snapshot;
            xQueueSend(xAudioQueue, &snap, 0); // non-blocking: descarta se queue cheia
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

	    if (frequency <= 0.0f) {
		display_service_showNoSignal();
		continue; 
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
        }
    }
}

