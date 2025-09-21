#include "dsp.h"
#include "fir_coeffs.h"
#include <math.h>


float32_t input_signal[FRAME_LEN];
float32_t filtered_signal[FRAME_LEN];
float32_t output_fft[FRAME_LEN];
arm_rfft_fast_instance_f32 fft_instance;
arm_fir_instance_f32 fir_instance;
float32_t fir_state[FRAME_LEN + NUM_TAPS - 1];


float dsp_last_peak = 0.0f;


#ifndef ADC_MAX_VAL
#define ADC_MAX_VAL 4095.0f
#endif

// remove DC via média, normaliza para ~[-1,1] e aplica Hanning
void preprocess_signal(volatile uint16_t *buffer, volatile float32_t *output) {
    float mean = 0.0f;

    // soma simples
    for (int i = 0; i < FRAME_LEN; i++) {
        mean += (float)buffer[i]; // FRAME_LEN;
    }
    mean /= (float)FRAME_LEN;

    // janela de Hanning + centraliza e normaliza
    for (int i = 0; i < FRAME_LEN; i++) {
        float window = 0.5f * (1 - cosf(2 * PI * i / (FRAME_LEN - 1)));
	float sample = (float)buffer[i] / ADC_MAX_VAL;   // normaliza para [0..1]
	output[i] = (sample - (mean / ADC_MAX_VAL)) * window;
        //output[i] = (((float)buffer[i] / FRAME_LEN) - mean) * window;
    }
}

// ===== FIR =====
void apply_fir(float32_t *input, float32_t *output) {
    arm_fir_f32(&fir_instance, input, output, FRAME_LEN);
}

// ===== FFT =====
void compute_fft(float32_t *input, float32_t *fft_out) {
    arm_rfft_fast_f32(&fft_instance, input, fft_out, 0);
}

// ===== MAGNITUDE =====
void compute_magnitude(float32_t *fft_out, float32_t *mag_out) {
    arm_cmplx_mag_f32(fft_out, mag_out, FRAME_LEN / 2);
}

// encontra índice do pico e retorna também o valor do pico via out_max
int find_peak_index(float32_t *spectrum, int len, float *out_max) {
    float max_value = 0.0f;
    int max_index = 0;
    int start_index = 5; // ignora bins muito baixos

    for (int i = start_index; i < len; i++) {
        if (spectrum[i] > max_value) {
            max_value = spectrum[i];
            max_index = i;
        }
    }
    if (out_max) *out_max = max_value;
    return max_index;
}

float index_to_frequency(int index, float sample_rate, int frame_len) {
    return index * sample_rate / frame_len;
}

// ===== HPS adaptativo (mantido) =====
void apply_hps_adaptive(float *magnitude_fft, float *hps_result, int hps_len,
                        float sample_rate, int frame_len) {
    for (int i = 0; i < hps_len; i++) {
	hps_result[i] = magnitude_fft[i];
    }

    float bin_hz = sample_rate / (float)frame_len;
    
    for (int i = 0; i < hps_len; i++) {
        float freq = i * bin_hz;
        int max_harm = 1;
	
        if (freq < 200.0f) {
	    max_harm = 3;
	}
        else if (freq < 600.0f) {
	    max_harm = 2;
	}

        // multiplicação direta (atenção overflow/underflow)
        for (int decimation = 2; decimation <= max_harm; decimation++) {
            int idx = i * decimation;
            if (idx < hps_len) {
                hps_result[i] *= magnitude_fft[idx];
            }
        }
    }
}

// ===== init =====
void dsp_init(void) {
    arm_rfft_fast_init_f32(&fft_instance, FRAME_LEN);
    arm_fir_init_f32(&fir_instance, NUM_TAPS, (float32_t *)fir_coeffs, fir_state, FRAME_LEN);

}

// ===== pipeline principal =====
// retorna 0 se não houver detecção robusta
float dsp_process(volatile uint16_t *buffer) {
    // 1) pré-processamento
    preprocess_signal(buffer, input_signal);

    // 2) FIR
    apply_fir(input_signal, filtered_signal);

    // 3) FFT
    compute_fft(filtered_signal, output_fft);

    // 4) magnitude
    compute_magnitude(output_fft, filtered_signal); // reuse filtered_signal como mag[N]

    // 5) opcional: HPS adaptativo (mantido)
    apply_hps_adaptive(filtered_signal, filtered_signal, FRAME_LEN / 2, SAMPLE_RATE, FRAME_LEN);

    // 6) encontrar pico e calcular magnitude média para limiar adaptativo
    float peak = 0.0f;
    int max_index = find_peak_index(filtered_signal, FRAME_LEN / 2, &peak);

    // média do espectro (usada para limiar)
    float mean_mag = 0.0f;
    int n = FRAME_LEN / 2;
    for (int i = 5; i < n; i++) mean_mag += filtered_signal[i];
    mean_mag /= (float)(n - 5);

    // limiar adaptativo: combinação de piso absoluto e múltiplo da média
    const float floor_abs = 1e-3f;             // piso mínimo (ajuste conforme necessidade)
    const float mult = 6.0f;                   // multiplicador sobre a média (ajuste)
    float threshold = fmaxf(floor_abs, mean_mag * mult);

    dsp_last_peak = peak;

    if (peak < threshold) {
        // nada detectado com confiança
        return 0.0f;
    }

    // 7) converter para frequência
    float freq = index_to_frequency(max_index, SAMPLE_RATE, FRAME_LEN);
    return freq;
}
