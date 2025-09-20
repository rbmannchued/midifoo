#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/f4/nvic.h>
#include <libopencm3/cm3/cortex.h>

#include "tuner.h"

#include "ssd1306.h"
#include "ssd1306_fonts.h"

#include <stdio.h>
#include <stdint.h>
#include <math.h>


SemaphoreHandle_t xUsartMutex;
SemaphoreHandle_t xBufferReadySemaphore;
QueueHandle_t xAudioQueue; // fila para enviar ponteiro do buffer para Task 2

TaskHandle_t xHandleAudioAcq = NULL;
TaskHandle_t xHandleFFTProc = NULL;
// resolution of 1.95hz
#define FRAME_LEN 2048
#define SAMPLE_RATE 4000
#define NUM_TAPS 64


volatile uint16_t adc_buffer1[FRAME_LEN];
volatile uint16_t adc_buffer2[FRAME_LEN];
volatile uint16_t *current_buffer = adc_buffer1;
volatile uint16_t *processing_buffer = adc_buffer2;

volatile int buffer_index = 0;
volatile int frame_ready = 0;

static volatile float32_t filtered_signal[FRAME_LEN];
static volatile float32_t input_signal[FRAME_LEN];

float magnitude_fft[FRAME_LEN / 2];
float input_fft[FRAME_LEN * 2];  // Entrada complexa (real + imaginária)
float output_fft[FRAME_LEN];      // Magnitude da FFT
arm_rfft_fast_instance_f32 fft_instance;
arm_fir_instance_f32 fir_instance;
float32_t fir_state[FRAME_LEN + NUM_TAPS - 1];

const float32_t fir_coeffs[64] = {
    -0.00019963,
    0.00046691,
    0.00035070,
    -0.00076233,
    -0.00207077,
    -0.00222905,
    -0.00083510,
    0.00048556,
    -0.00073123,
    -0.00475865,
    -0.00825601,
    -0.00727923,
    -0.00245701,
    -0.00001404,
    -0.00556880,
    -0.01639876,
    -0.02201229,
    -0.01516454,
    -0.00235712,
    0.00000327,
    -0.01658760,
    -0.03930061,
    -0.04323853,
    -0.01878963,
    0.01117643,
    0.00836715,
    -0.03936117,
    -0.09263793,
    -0.08430548,
    0.02052339,
    0.18146407,
    0.30212950,
    0.30212950,
    0.18146407,
    0.02052339,
    -0.08430548,
    -0.09263793,
    -0.03936117,
    0.00836715,
    0.01117643,
    -0.01878963,
    -0.04323853,
    -0.03930061,
    -0.01658760,
    0.00000327,
    -0.00235712,
    -0.01516454,
    -0.02201229,
    -0.01639876,
    -0.00556880,
    -0.00001404,
    -0.00245701,
    -0.00727923,
    -0.00825601,
    -0.00475865,
    -0.00073123,
    0.00048556,
    -0.00083510,
    -0.00222905,
    -0.00207077,
    -0.00076233,
    0.00035070,
    0.00046691,
    -0.00019963
};

const char *noteNames[] = {"A","A#","B","C","C#","D","D#","E","F","F#","G","G#"};
const float noteFrequencies[45] = {
  55.00,  // A1
  58.27,  // A#1
  61.74,  // B1
  65.41,  // C2
  69.30,  // C#2
  73.42,  // D2
  77.78,  // D#2
  82.41,  // E2
  87.31,  // F2
  92.50,  // F#2
  98.00,  // G2
  103.83, // G#2
  110.00, // A2
  116.54, // A#2
  123.47, // B2
  130.81, // C3
  138.59, // C#3
  146.83, // D3
  155.56, // D#3
  164.81, // E3
  174.61, // F3
  185.00, // F#3
  196.00, // G3
  207.65, // G#3
  220.00, // A3
  233.08, // A#3
  246.94, // B3
  261.63, // C4
  277.18, // C#4
  293.66, // D4
  311.13, // D#4
  329.63, // E4
  349.23, // F4
  369.99, // F#4
  392.00, // G4
  415.30, // G#4
  440.00, // A4
  466.16, // A#4
  493.88, // B4
  523.25, // C5
  554.37, // C#5
  587.33, // D5
  622.25, // D#5
  659.26, // E5
  698.46, // F5
};

int get_noteOctave(int noteIndex) {
    return (noteIndex / 12) + 1;
}
  
const int get_closestNoteIndex(double frequency){
    float minDiff = 1e9;
    int closestIndex = 0;

    for (int i = 1; i < sizeof(noteFrequencies) / sizeof(noteFrequencies[0]); i++) {
	float diff = fabs(frequency - noteFrequencies[i]);
	if (diff < minDiff) {
	    minDiff = diff;
	    closestIndex = i;
	}
    }
    return closestIndex;
    
}

const double get_noteDiff(double frequency, int closestIndex) {
    double noteGap;
    double frequencyGap;

    if (frequency < noteFrequencies[closestIndex]) {
        noteGap = noteFrequencies[closestIndex] - noteFrequencies[closestIndex - 1];
    } else {
        noteGap = noteFrequencies[closestIndex + 1] - noteFrequencies[closestIndex];
    }
  
    frequencyGap = fabs(frequency - noteFrequencies[closestIndex]) / noteGap * 100;
    if (frequency < noteFrequencies[closestIndex]) {
        frequencyGap = -frequencyGap;
    }
    return frequencyGap;
}

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
        uint16_t adc_value = adc_read_regular(ADC1); // Lê o valor do ADC

        // Insere no buffer atual
        current_buffer[buffer_index++] = adc_value;

        // Se o buffer atual estiver cheio, alterna para o outro buffer
        if (buffer_index >= FRAME_LEN) {
            buffer_index = 0;

            // Alterna os buffers
            volatile uint16_t *temp = current_buffer;
            current_buffer = processing_buffer;
            processing_buffer = temp;
	    
	    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xSemaphoreGiveFromISR(xBufferReadySemaphore, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
}

// Função HPS adaptativa para guitarra
// magnitude_fft  -> espectro de magnitude (entrada)
// hps_result     -> saída do HPS (ou cópia da magnitude para agudos)
// hps_len        -> tamanho do espectro (FRAME_LEN/2)
// sample_rate    -> taxa de amostragem (ex: 4000 Hz)
// frame_len      -> tamanho do frame (ex: 2048)

void apply_hps_adaptive(float *magnitude_fft, float *hps_result, int hps_len,
                        float sample_rate, int frame_len) {
    // Copia espectro base
    for (int i = 0; i < hps_len; i++) {
        hps_result[i] = magnitude_fft[i];
    }

    // Frequência de resolução (Hz por bin)
    float bin_hz = sample_rate / (float)frame_len;

    // Ajusta profundidade do HPS conforme a frequência (bin)
    for (int i = 0; i < hps_len; i++) {
        float freq = i * bin_hz;
        int max_harm = 1; // padrão = só magnitude (agudos)

        if (freq < 200.0f) {
            max_harm = 3;   // graves
        } else if (freq < 600.0f) {
            max_harm = 2;   // médios
        }

        // Aplica produto harmônico
        for (int decimation = 2; decimation <= max_harm; decimation++) {
            int idx = i * decimation;
            if (idx < hps_len) {
                hps_result[i] *= magnitude_fft[idx];
            }
        }
    }
}

 
void process_fft(volatile uint16_t *buffer) {
    char msg[65];
    float mean = 0.0f;

    int lastNoteIndex = 0;
    int lastNoteDiff = 0;
    double lastFrequency = 0;

    // Cálculo da média e normalização do sinal com janela Hanning
    for (int i = 0; i < FRAME_LEN; i++) {
        mean += (float)buffer[i] / FRAME_LEN;
    }
    mean /= FRAME_LEN;

    for (int i = 0; i < FRAME_LEN; i++) {
        float window = 0.5f * (1 - cosf(2 * PI * i / (FRAME_LEN - 1)));  
        input_signal[i] = (((float)buffer[i] / FRAME_LEN) - mean) * window;  
    }

    //aplica o filtro fir
    arm_fir_f32(&fir_instance, input_signal, filtered_signal, FRAME_LEN);
    // Executar FFT
    arm_rfft_fast_f32(&fft_instance, filtered_signal, output_fft, 0);

    // Calcular magnitude da FFT e aplicar HPS diretamente no mesmo array
    arm_cmplx_mag_f32(output_fft, filtered_signal, FRAME_LEN / 2);
//    apply_hps(filtered_signal, filtered_signal, FRAME_LEN / 2);
    apply_hps_adaptive(filtered_signal, filtered_signal, FRAME_LEN / 2, SAMPLE_RATE, FRAME_LEN);

    // Encontrar pico máximo
    float max_value = 0.0f;
    int max_index = 0;
    int start_index = 5;

    for (int i = start_index; i < FRAME_LEN / 2; i++) {
        if (filtered_signal[i] > max_value) {
            max_value = filtered_signal[i];
            max_index = i;
        }
    }

    // Ignorar picos pequenos
    if (max_value < 0.1f) {
        max_index = 0;
    }

    // Converter índice para frequência
    float frequency = (float)max_index * SAMPLE_RATE / FRAME_LEN;
    int noteIndex = get_closestNoteIndex(frequency);

    if (frequency == 0 || noteIndex == 60 || noteIndex == 0) {
        display_result(lastNoteDiff, lastFrequency, lastNoteIndex);
    } else {
        int noteDiff = get_noteDiff(frequency, noteIndex);
        display_result(noteDiff, frequency, noteIndex);
        lastNoteDiff = noteDiff;
        lastFrequency = frequency;
        lastNoteIndex = noteIndex;
    }

    // Enviar para UART
    /* snprintf(msg, sizeof(msg), "Index: %d, Freq: %.2f Hz, nota mais prox: %d \r\n", max_index, frequency, noteIndex); */
    /* usart_send_string(msg); */
}

void timer_init(void) {
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

void tuner_init(){
    rcc_periph_clock_enable(RCC_GPIOA);
    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO6);
    
    
    cm_enable_interrupts();     
    adc_init();
    timer_init();
    rcc_periph_clock_enable(GPIOA);
    rcc_periph_clock_enable(GPIOC);

    arm_rfft_fast_init_f32(&fft_instance, FRAME_LEN);
    arm_fir_init_f32(&fir_instance, NUM_TAPS, (float32_t *)fir_coeffs, fir_state, FRAME_LEN);
    
    xBufferReadySemaphore = xSemaphoreCreateBinary();
    xAudioQueue = xQueueCreate(2, sizeof(uint16_t *));
    
    /* ssd1306_Fill(Black); */
    /* ssd1306_WriteString("...", Font_16x26,White); */
    /* ssd1306_UpdateScreen(); */
    
}

void audio_start(void) {
    // Reinicia o ADC
    adc_power_off(ADC1);
    adc_power_on(ADC1);
    
    // Reinicia o timer
    timer_disable_counter(TIM2);
    timer_enable_counter(TIM2);
    
    adc_start_conversion_regular(ADC1);
    vTaskResume(xHandleAudioAcq);
    vTaskResume(xHandleFFTProc);
}

void audio_stop(void) {
    timer_disable_counter(TIM2);
    adc_power_off(ADC1);
    vTaskSuspend(xHandleAudioAcq);
    vTaskSuspend(xHandleFFTProc);

    // Limpa buffers e estados
    buffer_index = 0;
    frame_ready = 0;
    /* freq_filtered = 0.0f; */
    /* last_shown_freq = 0.0f; */
}



void vTaskAudioAcquisition(void *pvParameters) {
    for (;;) {
        // Espera até a ISR sinalizar que um frame esta pronto
        if (xSemaphoreTake(xBufferReadySemaphore, portMAX_DELAY) == pdTRUE) {
            // Envia o ponteiro do buffer pronto para a Task 2
            xQueueSend(xAudioQueue, &processing_buffer, portMAX_DELAY);
        }
    }
}

void vTaskFFTProcessing(void *pvParameters) {
    uint16_t *buffer_ptr;

    for (;;) {
        // Espera um buffer ficar disponível
        if (xQueueReceive(xAudioQueue, &buffer_ptr, portMAX_DELAY) == pdTRUE) {
            process_fft(buffer_ptr);
	     vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}
