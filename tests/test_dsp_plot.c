// test_dsp_plot.c
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sndfile.h>
#include "dsp.h"

#define STEP 4  // reduzir número de pontos para plot

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Uso: %s samples/E_82_strato_neck.wav\n", argv[0]);
        return 1;
    }

    // ---- abrir WAV ----
    SNDFILE *sf;
    SF_INFO info;
    info.format = 0;
    sf = sf_open(argv[1], SFM_READ, &info);
    if (!sf) {
        printf("Erro abrindo %s\n", argv[1]);
        return 1;
    }

    printf("Arquivo: %s | SR=%d | canais=%d | frames=%ld\n",
           argv[1], info.samplerate, info.channels, (long)info.frames);

    float wav_buf[FRAME_LEN];
    int readcount = sf_readf_float(sf, wav_buf, FRAME_LEN);
    sf_close(sf);

    if (readcount < FRAME_LEN) {
        printf("WAV muito curto!\n");
        return 1;
    }

    // Simular ADC 12-bit
    static uint16_t adc_buf[FRAME_LEN];
    for (int i = 0; i < FRAME_LEN; i++) {
        int val = (int)((wav_buf[i] + 1.0f) * (ADC_MAX_VAL / 2.0f));
        if (val < 0) val = 0;
        if (val > (int)ADC_MAX_VAL) val = (int)ADC_MAX_VAL;
        adc_buf[i] = (uint16_t)val;
    }

    // ---- inicializar DSP ----
    dsp_init();

    // ---- pipeline principal ----
    float freq_detected = dsp_process(adc_buf);
    printf("Frequência detectada: %.2f Hz\n", freq_detected);

    // ---- salvar sinais de tempo ----
    FILE *f_in = fopen("time_input.dat", "w");
    FILE *f_fir = fopen("time_filtered.dat", "w");
    for (int i = 0; i < FRAME_LEN; i += STEP) {
        fprintf(f_in, "%d %f\n", i, input_signal[i]);
        fprintf(f_fir, "%d %f\n", i, filtered_signal[i]);
    }
    fclose(f_in);
    fclose(f_fir);

    // ---- FFT do sinal original (pré-FIR) ----
    float32_t fft_input[FRAME_LEN];
    float32_t mag_input[FRAME_LEN/2];
    compute_fft(input_signal, fft_input);
    compute_magnitude(fft_input, mag_input);

    FILE *f_fft_in = fopen("fft_mag_input.dat", "w");
    for (int i = 0; i < FRAME_LEN/2; i++) {
        float freq_bin = index_to_frequency(i, SAMPLE_RATE, FRAME_LEN);
        fprintf(f_fft_in, "%f %f\n", freq_bin, mag_input[i]);
    }
    fclose(f_fft_in);

    // ---- FFT pós-FIR + HPS ----
    FILE *f_fft_fir = fopen("fft_mag_filtered.dat", "w");
    for (int i = 0; i < FRAME_LEN/2; i++) {
        float freq_bin = index_to_frequency(i, SAMPLE_RATE, FRAME_LEN);
        fprintf(f_fft_fir, "%f %f\n", freq_bin, filtered_signal[i]); // filtered_signal já tem HPS aplicado
    }
    fclose(f_fft_fir);

    // ---- salvar HPS separadamente ----
    FILE *f_hps = fopen("hps.dat", "w");
    float hps_result[FRAME_LEN/2];
    apply_hps_adaptive(mag_input, hps_result, FRAME_LEN/2, SAMPLE_RATE, FRAME_LEN);
    for (int i = 0; i < FRAME_LEN/2; i++) {
        float freq_bin = index_to_frequency(i, SAMPLE_RATE, FRAME_LEN);
        fprintf(f_hps, "%f %f\n", freq_bin, hps_result[i]);
    }
    fclose(f_hps);

    printf("Arquivos gerados: time_input.dat, time_filtered.dat, fft_mag_input.dat, fft_mag_filtered.dat, hps.dat\n");
    printf("Exemplo de gnuplot:\n");
    printf("  gnuplot -persist -e \"plot 'time_input.dat' w l title 'Input'; plot 'time_filtered.dat' w l title 'Filtered'\"\n");
    printf("  gnuplot -persist -e \"plot 'fft_mag_input.dat' w l title 'FFT Input'; plot 'fft_mag_filtered.dat' w l title 'FFT Filtered'; plot 'hps.dat' w l title 'HPS'\n");

    return 0;
}
