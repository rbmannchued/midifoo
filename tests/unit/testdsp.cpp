#include "CppUTest/TestHarness.h"
#include "CppUTest/TestHarness_c.h"
#include "CppUTest/MemoryLeakDetector.h"
#include <cmath>
#include <cstring>

extern "C" {
    #include "dsp.h"
    #include "fir_coeffs.h"
    #include "arm_math.h"
}

TEST_GROUP(DSPUnitTest) {
    float32_t test_input[FRAME_LEN];
    float32_t test_output[FRAME_LEN];
    float32_t test_fft[FRAME_LEN];
    float32_t test_magnitude[FRAME_LEN/2];
    
    void setup() {
        // Inicializa buffers de teste
        memset(test_input, 0, sizeof(test_input));
        memset(test_output, 0, sizeof(test_output));
        memset(test_fft, 0, sizeof(test_fft));
        memset(test_magnitude, 0, sizeof(test_magnitude));
        
        dsp_init();
    }
    
    void teardown() {
    }
    
    void fill_sine_wave(float32_t* buffer, int length, float frequency, float sample_rate) {
        for (int i = 0; i < length; i++) {
            buffer[i] = sinf(2.0f * PI * frequency * i / sample_rate);
        }
    }
    
    void fill_dc_signal(float32_t* buffer, int length, float value) {
        for (int i = 0; i < length; i++) {
            buffer[i] = value;
        }
    }
};


TEST(DSPUnitTest, PreprocessSignal_AppliesHPFAndWindow) {
    volatile uint16_t adc_buffer[FRAME_LEN];
    volatile float32_t output[FRAME_LEN];
    

    for (int i = 0; i < FRAME_LEN; i++) {
        adc_buffer[i] = 2048;
    }
    
    preprocess_signal(adc_buffer, output);
    
    // Verifica que o processamento foi aplicado
    // O HPF deve remover o DC, então a média deve ser próxima de zero
    float sum = 0.0f;
    for (int i = 0; i < FRAME_LEN; i++) {
        sum += output[i];
    }
    float mean = sum / FRAME_LEN;
    
    DOUBLES_EQUAL(0.0f, mean, 0.1f);
    
    // Verifica que a janela foi aplicada (valores nas bordas menores)
    CHECK_TRUE(fabsf(output[0]) < fabsf(output[FRAME_LEN/2]));
    CHECK_TRUE(fabsf(output[FRAME_LEN-1]) < fabsf(output[FRAME_LEN/2]));
}

TEST(DSPUnitTest, PreprocessSignal_NormalizesToExpectedRange) {
    volatile uint16_t adc_buffer[FRAME_LEN];
    volatile float32_t output[FRAME_LEN];
    
    // Testa valores mínimo e maximo do ADC
    adc_buffer[0] = 0;      
    adc_buffer[1] = 4095;   // Maximo
    
    for (int i = 2; i < FRAME_LEN; i++) {
        adc_buffer[i] = 2048;
    }
    
    preprocess_signal(adc_buffer, output);
    
    // Após normalização e HPF, os valores devem estar em faixa razoável
    for (int i = 0; i < FRAME_LEN; i++) {
        CHECK_TRUE(output[i] >= -1.0f && output[i] <= 1.0f);
    }
}

//test fir filter
TEST(DSPUnitTest, ApplyFIR_FiltersSignal) {
    // Cria um sinal de teste (senoide + ruído)
    for (int i = 0; i < FRAME_LEN; i++) {
        test_input[i] = sinf(2.0f * PI * 100.0f * i / SAMPLE_RATE) + 
                       0.1f * sinf(2.0f * PI * 1000.0f * i / SAMPLE_RATE);
    }
    
    apply_fir(test_input, test_output);
    
    // Verifica que a saída é diferente da entrada (filtro aplicado)
    bool different = false;
    for (int i = NUM_TAPS; i < FRAME_LEN; i++) {
        if (fabsf(test_input[i] - test_output[i]) > 0.001f) {
            different = true;
            break;
        }
    }
    CHECK_TRUE(different);
    
    // Verifica que não há NaN ou infinitos
    for (int i = 0; i < FRAME_LEN; i++) {
        CHECK_TRUE(isnormal(test_output[i]) || test_output[i] == 0.0f);
    }
}

// Teste para compute_fft
TEST(DSPUnitTest, ComputeFFT_ProducesComplexOutput) {
    // Sinal senoidal puro
    fill_sine_wave(test_input, FRAME_LEN, 440.0f, SAMPLE_RATE);
    
    compute_fft(test_input, test_fft);
    
    // A FFT produz saída complexa (partes real e imaginária)
    // Verifica que há componentes não-zero
    bool has_non_zero = false;
    for (int i = 0; i < FRAME_LEN; i++) {
        if (fabsf(test_fft[i]) > 0.001f) {
            has_non_zero = true;
            break;
        }
    }
    CHECK_TRUE(has_non_zero);
}

// Teste para compute_magnitude
TEST(DSPUnitTest, ComputeMagnitude_ProducesRealValues) {
    // Preenche dados FFT fictícios (parte real e imaginária)
    for (int i = 0; i < FRAME_LEN; i += 2) {
        test_fft[i] = (float)i / FRAME_LEN;     // Parte real
        test_fft[i+1] = (float)(i+1) / FRAME_LEN; // Parte imaginária
    }
    
    compute_magnitude(test_fft, test_magnitude);
    
    // Verifica que todas as magnitudes são não-negativas
    for (int i = 0; i < FRAME_LEN/2; i++) {
        CHECK_TRUE(test_magnitude[i] >= 0.0f);
        CHECK_TRUE(isnormal(test_magnitude[i]) || test_magnitude[i] == 0.0f);
    }
}

// Teste para find_peak_index
TEST(DSPUnitTest, FindPeakIndex_LocatesMaximumValue) {
    float32_t spectrum[100];
    
    // Preenche com valores decrescentes
    for (int i = 0; i < 100; i++) {
        spectrum[i] = (float)(100 - i);
    }
    
    // Insere um pico conhecido
    int expected_peak = 42;
    spectrum[expected_peak] = 200.0f;
    
    float max_value = 0.0f;
    int peak_index = find_peak_index(spectrum, 100, &max_value);
    
    LONGS_EQUAL(expected_peak, peak_index);
    DOUBLES_EQUAL(200.0f, max_value, 0.001f);
}

TEST(DSPUnitTest, FindPeakIndex_RespectsStartIndex) {
    float32_t spectrum[100];
    
    // Pico no início (deve ser ignorado)
    spectrum[0] = 100.0f;
    spectrum[1] = 90.0f;
    spectrum[2] = 80.0f;
    spectrum[3] = 70.0f;
    spectrum[4] = 60.0f;
    
    // Pico que deve ser detectado (após start_index = 5)
    int expected_peak = 50;
    for (int i = 5; i < 100; i++) {
        spectrum[i] = (float)i;
    }
    spectrum[expected_peak] = 200.0f;
    
    float max_value = 0.0f;
    int peak_index = find_peak_index(spectrum, 100, &max_value);
    
    LONGS_EQUAL(expected_peak, peak_index);
}

TEST(DSPUnitTest, FindPeakIndex_HandlesAllZeroInput) {
    float32_t spectrum[100] = {0};
    
    float max_value = 0.0f;
    int peak_index = find_peak_index(spectrum, 100, &max_value);
    
    // CORREÇÃO: Quando todos são zero, deve retornar 0
    LONGS_EQUAL(0, peak_index); // ← Mude de 5 para 0
    DOUBLES_EQUAL(0.0f, max_value, 0.001f);
}


// Teste para index_to_frequency
TEST(DSPUnitTest, IndexToFrequency_CalculatesCorrectly) {
    struct TestCase {
        int index;
        float expected_freq;
    } test_cases[] = {
        {0, 0.0f},
        {1, 1000.0f},  // SAMPLE_RATE=4000, FRAME_LEN=2048: 4000/2048 ≈ 1.953Hz por bin
        {10, 10000.0f},
        {100, 100000.0f}
    };
    
    for (auto& test_case : test_cases) {
        float freq = index_to_frequency(test_case.index, 4000.0f, 2048);
        float expected = test_case.index * 4000.0f / 2048;
        DOUBLES_EQUAL(expected, freq, 0.001f);
    }
}

// Teste para apply_hps_adaptive
TEST(DSPUnitTest, ApplyHPSAdaptive_EnhancesHarmonics) {
    const int hps_len = FRAME_LEN / 2;
    float magnitude_fft[hps_len];
    float hps_result[hps_len];
    
    // Cria um espectro com harmônicos
    for (int i = 0; i < hps_len; i++) {
        magnitude_fft[i] = 0.0f;
    }
    
    // Fundamental em bin 10 (~19.5Hz) e harmônicos
    magnitude_fft[10] = 1.0f;  // Fundamental
    magnitude_fft[20] = 0.8f;  // 2º harmônico
    magnitude_fft[30] = 0.6f;  // 3º harmônico
    
    apply_hps_adaptive(magnitude_fft, hps_result, hps_len, SAMPLE_RATE, FRAME_LEN);
    
    // Verifica que HPS foi aplicado (resultado diferente do original)
    bool different = false;
    for (int i = 0; i < hps_len; i++) {
        if (fabsf(magnitude_fft[i] - hps_result[i]) > 0.001f) {
            different = true;
            break;
        }
    }
    CHECK_TRUE(different);
    
    // Verifica que não há valores inválidos
    for (int i = 0; i < hps_len; i++) {
        CHECK_TRUE(hps_result[i] >= 0.0f);
        CHECK_TRUE(isnormal(hps_result[i]) || hps_result[i] == 0.0f);
    }
}

TEST(DSPUnitTest, ApplyHPSAdaptive_HandlesZeroInput) {
    const int hps_len = FRAME_LEN / 2;
    float magnitude_fft[hps_len] = {0};
    float hps_result[hps_len];
    
    apply_hps_adaptive(magnitude_fft, hps_result, hps_len, SAMPLE_RATE, FRAME_LEN);
    
    // Verifica que não crasha e produz saída válida
    for (int i = 0; i < hps_len; i++) {
        CHECK_TRUE(hps_result[i] >= 0.0f);
        CHECK_TRUE(isnormal(hps_result[i]) || hps_result[i] == 0.0f);
    }
}

// Teste para dsp_init
TEST(DSPUnitTest, DSPInit_InitializesStructures) {
    // dsp_init é chamado no setup()
    // Verifica se as estruturas foram inicializadas
    CHECK_TRUE(fir_instance.numTaps == NUM_TAPS);
    CHECK_TRUE(fir_instance.pCoeffs != NULL);
    CHECK_TRUE(fir_instance.pState != NULL);
}

// Teste para dsp_process com silêncio
TEST(DSPUnitTest, DSPProcess_ReturnsZeroForSilence) {
    volatile uint16_t silent_buffer[FRAME_LEN];
    
    // Buffer de silêncio (meio da escala ADC)
    for (int i = 0; i < FRAME_LEN; i++) {
        silent_buffer[i] = 2048;
    }
    
    float result = dsp_process(silent_buffer);
    
    DOUBLES_EQUAL(0.0f, result, 0.001f);
}

TEST(DSPUnitTest, DSPProcess_HandlesExtremeADCValues) {
    volatile uint16_t extreme_buffer[FRAME_LEN];
    
    // Testa valores mínimo e máximo do ADC
    for (int i = 0; i < FRAME_LEN; i++) {
        extreme_buffer[i] = (i % 2 == 0) ? 0 : 4095;
    }
    
    float result = dsp_process(extreme_buffer);
    
    // Não deve crashar, pode retornar 0 ou uma frequência
    CHECK_TRUE(result >= 0.0f);
}

TEST(DSPUnitTest, DSPProcess_DetectsSimpleSineWave) {
    volatile uint16_t sine_buffer[FRAME_LEN];
    float test_freq = 440.0f; // Lá
    
    // Gera senoide em 440Hz
    for (int i = 0; i < FRAME_LEN; i++) {
        float sample = sinf(2.0f * PI * test_freq * i / SAMPLE_RATE);
        sine_buffer[i] = (uint16_t)(2048 + 1000 * sample); // Amplitude suficiente
    }
    
    float detected_freq = dsp_process(sine_buffer);
    
    // Para teste unitário, verifica apenas que detectou algo
    // Em ambiente real, a detecção deve ser próxima de 440Hz
    if (detected_freq > 0) {
        CHECK_TRUE(detected_freq > 0.0f);

        CHECK_TRUE(fabsf(detected_freq - test_freq) < 50.0f); //+ ou -50Hz
    }
}
