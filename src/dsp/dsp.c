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

/* Applies Hanning and normalize */
void preprocess_signal(volatile uint16_t *buffer, volatile float32_t *output) {
    // IIR HPF: y[n] = alpha * (y[n-1] + x[n] - x[n-1])
    const float alpha = 0.995f;
    static float x_prev = 0.0f, y_prev = 0.0f;
    for (int i = 0; i < FRAME_LEN; i++) {
        float x = ((float)buffer[i] / ADC_MAX_VAL); // normalize 0..1
        float y = alpha * (y_prev + x - x_prev);
        x_prev = x;
        y_prev = y;
        float window = 0.5f * (1.0f - cosf(2.0f * PI * i / (FRAME_LEN - 1)));
        output[i] = y * window;
    }
}

void apply_fir(float32_t *input, float32_t *output) {
    arm_fir_f32(&fir_instance, input, output, FRAME_LEN);
}

void compute_fft(float32_t *input, float32_t *fft_out) {
    arm_rfft_fast_f32(&fft_instance, input, fft_out, 0);
}

void compute_magnitude(float32_t *fft_out, float32_t *mag_out) {
    arm_cmplx_mag_f32(fft_out, mag_out, FRAME_LEN / 2);
}

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
void apply_hps_adaptive(float *magnitude_fft, float *hps_result, int hps_len,
                        float sample_rate, int frame_len) {
    const float EPS = 1e-9f;
    float bin_hz = sample_rate / (float)frame_len;

    for (int i = 0; i < hps_len; i++) {
        float freq = i * bin_hz;
        int max_harm = 1;

        // Menos agressivo em tons graves
        if (freq < 120.0f) {
            max_harm = 3;
        } else if (freq < 300.0f) {
            max_harm = 2;
        } else {
            max_harm = 1;
        }

        float acc = 0.0f;
        int count = 0;

        for (int decimation = 1; decimation <= max_harm; decimation++) {
            int idx = i * decimation;
            if (idx < hps_len) {
                acc += logf(magnitude_fft[idx] + EPS);
                count++;
            } else break;
        }

        // média logarítmica (normaliza pela contagem)
        hps_result[i] = expf(acc / (float)count);
    }

    // mistura 70% HPS e 30% espectro original para estabilidade
    for (int i = 0; i < hps_len; i++) {
        hps_result[i] = 0.7f * hps_result[i] + 0.3f * magnitude_fft[i];
    }
}

void dsp_init(void) {
    arm_rfft_fast_init_f32(&fft_instance, FRAME_LEN);
    arm_fir_init_f32(&fir_instance, NUM_TAPS, (float32_t *)fir_coeffs, fir_state, FRAME_LEN);

}

float dsp_process(volatile uint16_t *buffer) {

    preprocess_signal(buffer, input_signal);

    apply_fir(input_signal, filtered_signal);

    compute_fft(filtered_signal, output_fft);

    compute_magnitude(output_fft, filtered_signal);

    apply_hps_adaptive(filtered_signal, filtered_signal, FRAME_LEN / 2, SAMPLE_RATE, FRAME_LEN);

    float peak = 0.0f;
    int max_index = find_peak_index(filtered_signal, FRAME_LEN / 2, &peak);

    float mean_mag = 0.0f;
    int n = FRAME_LEN / 2;
    for (int i = 5; i < n; i++) mean_mag += filtered_signal[i];
    mean_mag /= (float)(n - 5);

    const float floor_abs = 1e-3f;            
    const float mult = 6.0f;                  
    float threshold = fmaxf(floor_abs, mean_mag * mult);

    dsp_last_peak = peak;

    if (peak < threshold) {
        return 0.0f;
    }


    float freq = index_to_frequency(max_index, SAMPLE_RATE, FRAME_LEN);

// --- correção de harmônicos múltiplos ---
    float fundamental = freq;
    float half_freq = freq * 0.5f;
    float third_freq = freq / 3.0f;
    float half_bin = half_freq * FRAME_LEN / SAMPLE_RATE;
    float third_bin = third_freq * FRAME_LEN / SAMPLE_RATE;

// verifica se há energia considerável em sub-harmônicos
    if (half_bin > 5 && filtered_signal[(int)half_bin] > 0.4f * dsp_last_peak) {
	fundamental = half_freq;
    } else if (third_bin > 5 && filtered_signal[(int)third_bin] > 0.4f * dsp_last_peak) {
	fundamental = third_freq;
    }

    return fundamental;
}
