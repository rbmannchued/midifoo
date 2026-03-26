#include "dsp.h"
#include "fir_coeffs.h"
#include <math.h>


float32_t input_signal[FRAME_LEN];
float32_t filtered_signal[FRAME_LEN];
float32_t output_fft[FRAME_LEN];
arm_rfft_fast_instance_f32 fft_instance;
arm_fir_instance_f32 fir_instance;
float32_t fir_state[FRAME_LEN + NUM_TAPS - 1];
static float32_t hanning_window[FRAME_LEN / 2]; // simetria: w[i] == w[N-1-i], armazena só metade (8KB)


float dsp_last_peak = 0.0f;


#ifndef ADC_MAX_VAL
#define ADC_MAX_VAL 4095.0f
#endif

#define SILENCE_THRESHOLD 0.007f

/* Applies Hanning and normalize */
void preprocess_signal(volatile uint16_t *buffer, volatile float32_t *output) {
    // IIR HPF: y[n] = alpha * (y[n-1] + x[n] - x[n-1])
    const float alpha = 0.97f;
    static float x_prev = 0.0f, y_prev = 0.0f;
    for (int i = 0; i < FRAME_LEN; i++) {
        float x = ((float)buffer[i] / ADC_MAX_VAL); // normalize 0..1
        float y = alpha * (y_prev + x - x_prev);
        x_prev = x;
        y_prev = y;
        int wi = (i < FRAME_LEN / 2) ? i : (FRAME_LEN - 1 - i);
        output[i] = y * hanning_window[wi]; // janela pré-computada (simetria)
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
    const int start_index = 5; // ignora bins muito baixos
    float max_value;
    uint32_t max_idx;
    arm_max_f32(spectrum + start_index, len - start_index, &max_value, &max_idx);
    if (out_max) *out_max = max_value;
    return (int)(max_idx + start_index);
}

float index_to_frequency(int index, float sample_rate, int frame_len) {
    return index * sample_rate / frame_len;
}
void apply_hps_adaptive(float *magnitude_fft, float *hps_result, int hps_len,
                        float sample_rate, int frame_len) {
    const float EPS = 1e-9f;
    float bin_hz = sample_rate / (float)frame_len;

    // Limites de bin pré-calculados para evitar multiplicação por float no loop
    const int bin_120hz = (int)(120.0f / bin_hz);  // ~122 bins
    const int bin_300hz = (int)(350.0f / bin_hz);  // ~358 bins — covers E4 (330Hz) with 2-harmonic HPS

    // Zona max_harm=3: média geométrica = cbrt(a*b*c) — sem logf/expf
    for (int i = 0; i < bin_120hz && i < hps_len; i++) {
        float a = magnitude_fft[i] + EPS;
        int i2 = i * 2, i3 = i * 3;
        float b = (i2 < hps_len) ? magnitude_fft[i2] + EPS : EPS;
        float c = (i3 < hps_len) ? magnitude_fft[i3] + EPS : EPS;
        float hps_val = cbrtf(a * b * c);
        hps_result[i] = 0.7f * hps_val + 0.3f * magnitude_fft[i];
    }

    // Zona max_harm=2: média geométrica = sqrt(a*b) — arm_sqrt_f32 é instrução única no Cortex-M4
    for (int i = bin_120hz; i < bin_300hz && i < hps_len; i++) {
        float a = magnitude_fft[i] + EPS;
        int i2 = i * 2;
        float b = (i2 < hps_len) ? magnitude_fft[i2] + EPS : EPS;
        float hps_val;
        arm_sqrt_f32(a * b, &hps_val);
        hps_result[i] = 0.7f * hps_val + 0.3f * magnitude_fft[i];
    }

    // Zona max_harm=1: HPS = própria magnitude (sem operação adicional)
    for (int i = bin_300hz; i < hps_len; i++) {
        float orig = magnitude_fft[i];
        hps_result[i] = orig + 0.7f * EPS; // 0.7*hps + 0.3*orig com hps=orig+EPS
    }
}

void dsp_init(void) {
    arm_rfft_fast_init_f32(&fft_instance, FRAME_LEN);
    arm_fir_init_f32(&fir_instance, NUM_TAPS, (float32_t *)fir_coeffs, fir_state, FRAME_LEN);

    // Pré-computa metade da janela Hanning (é simétrica: w[i] == w[N-1-i])
    for (int i = 0; i < FRAME_LEN / 2; i++) {
        hanning_window[i] = 0.5f * (1.0f - cosf(2.0f * PI * i / (FRAME_LEN - 1)));
    }
}

float dsp_process(volatile uint16_t *buffer) {

    preprocess_signal(buffer, input_signal);

    float abs_sum = 0.0f;
    for (int i = 0; i < FRAME_LEN; i += 4) { // amostragem parcial (1/4 das amostras)
        abs_sum += fabsf(input_signal[i]);
    }
    float mean_abs = abs_sum / (FRAME_LEN / 4);

    if (mean_abs < SILENCE_THRESHOLD) {
        return 0.0f; // sem sinal detectável
    }

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

    const float floor_abs = 3e-4f;
    const float mult = 2.5f;
    float threshold = fmaxf(floor_abs, mean_mag * mult);

    dsp_last_peak = peak;

    if (peak < threshold) {
        return 0.0f;
    }


    float freq = index_to_frequency(max_index, SAMPLE_RATE, FRAME_LEN);


    float fundamental = freq;
    float half_freq = freq * 0.5f;
    float third_freq = freq / 3.0f;
    float half_bin = half_freq * FRAME_LEN / SAMPLE_RATE;
    float third_bin = third_freq * FRAME_LEN / SAMPLE_RATE;

    //verifies octave bins — check third (freq/3) before half (freq/2) to prefer
    // the lower fundamental (e.g. A2 at 110Hz detected via E4 at 330Hz, not E3 at 165Hz)
    if (third_bin > 5 && filtered_signal[(int)third_bin] > 0.3f * dsp_last_peak) {
	fundamental = third_freq;
    } else if (half_bin > 5 && filtered_signal[(int)half_bin] > 0.4f * dsp_last_peak) {
	fundamental = half_freq;
    }

    return fundamental;
}
