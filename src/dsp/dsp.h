#ifndef DSP_H
#define DSP_H

#include <stdint.h>
#include "arm_math.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FRAME_LEN     2048
#define SAMPLE_RATE   4000.0f
#define NUM_TAPS      64
#define ADC_MAX_VAL   4095.0f

extern float32_t input_signal[FRAME_LEN];
extern float32_t filtered_signal[FRAME_LEN];
extern float32_t output_fft[FRAME_LEN];

extern arm_rfft_fast_instance_f32 fft_instance;
extern arm_fir_instance_f32 fir_instance;
extern float32_t fir_state[FRAME_LEN + NUM_TAPS - 1];

void dsp_init(void);
float dsp_process(volatile uint16_t *buffer);
void preprocess_signal(volatile uint16_t *buffer, volatile float32_t *output);
void apply_fir(float32_t *input, float32_t *output);
void compute_fft(float32_t *input, float32_t *fft_out);
void compute_magnitude(float32_t *fft_out, float32_t *mag_out);
int find_peak_index(float32_t *spectrum, int len, float *out_max);
float index_to_frequency(int index, float sample_rate, int frame_len);
void apply_hps_adaptive(float *magnitude_fft,
                        float *hps_result,
                        int hps_len,
                        float sample_rate,
                        int frame_len);

#ifdef __cplusplus
}
#endif

#endif // DSP_H
