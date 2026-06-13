#include "test_dsp.h"

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define TEST_ENVIRONMENT

// Tolerância para comparação de frequências (em Hz)
#define FREQUENCY_TOLERANCE 1.0f

// Estrutura para header WAV simplificado
#pragma pack(push, 1)
typedef struct {
    char chunkId[4];
    uint32_t chunkSize;
    char format[4];
    char subchunk1Id[4];
    uint32_t subchunk1Size;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    char subchunk2Id[4];
    uint32_t subchunk2Size;
} WavHeader;
#pragma pack(pop)

// Função para carregar arquivo WAV
void load_wav_file(const char* filename, uint16_t** buffer, int* size) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        FAIL("Failed to open WAV file");
        return;
    }

    WavHeader header;
    fread(&header, sizeof(WavHeader), 1, file);

    // Verifica se é um arquivo WAV válido
    if (memcmp(header.chunkId, "RIFF", 4) != 0 ||
        memcmp(header.format, "WAVE", 4) != 0) {
        fclose(file);
        FAIL("Invalid WAV file");
        return;
    }

    printf("WAV Info: %d Hz, %d channels, %d-bit\n", 
           header.sampleRate, header.numChannels, header.bitsPerSample);

    // Calcula número de amostras
    int total_bytes = header.subchunk2Size;
    int bytes_per_sample = header.bitsPerSample / 8;
    int samples_per_channel = total_bytes / (bytes_per_sample * header.numChannels);
    
    *size = samples_per_channel;
    *buffer = (uint16_t*)malloc(*size * sizeof(uint16_t));
    
    if (header.numChannels == 1 && header.bitsPerSample == 16) {
        // Mono 16-bit - lê diretamente
        fread(*buffer, sizeof(uint16_t), *size, file);
    }
    else if (header.bitsPerSample == 32) {
        // 32-bit float - converte para 12-bit
        float* float_buffer = (float*)malloc(samples_per_channel * header.numChannels * sizeof(float));
        fread(float_buffer, sizeof(float), samples_per_channel * header.numChannels, file);
        
        for (int i = 0; i < samples_per_channel; i++) {
            float sample;
            if (header.numChannels == 1) {
                sample = float_buffer[i];
            } else {
                sample = (float_buffer[i * 2] + float_buffer[i * 2 + 1]) / 2.0f;
            }
            // Normaliza de -1.0..1.0 para 0..4095
            sample = fmaxf(-1.0f, fminf(1.0f, sample)); // Clamp
            (*buffer)[i] = (uint16_t)((sample + 1.0f) * 2047.5f);
        }
        
        free(float_buffer);
    }
    else {
        // Outros formatos - tentativa genérica
        printf("Warning: Unsupported format - attempting generic conversion\n");
        uint8_t* raw_buffer = (uint8_t*)malloc(total_bytes);
        fread(raw_buffer, 1, total_bytes, file);
        
        // Simples conversão (pode precisar de ajustes)
        for (int i = 0; i < samples_per_channel; i++) {
            // Pega primeiro canal ou média
            uint16_t sample = 0;
            if (header.bitsPerSample == 8) {
                sample = raw_buffer[i * header.numChannels] * 16;
            } else if (header.bitsPerSample == 16) {
                int16_t* samples_16 = (int16_t*)raw_buffer;
                sample = (samples_16[i * header.numChannels] + 32768) * 4095.0f / 65535.0f;
            }
            (*buffer)[i] = sample;
        }
        
        free(raw_buffer);
    }

    fclose(file);
}

// Função para processar arquivo WAV e obter frequência detectada
float test_dsp_process_with_wav(const char* filename) {
    uint16_t* buffer = NULL;
    int total_samples = 0;
    
    load_wav_file(filename, &buffer, &total_samples);
    
    if (!buffer || total_samples == 0) {
        return 0.0f;
    }

    // Inicializa o DSP
    dsp_init();
    
    // Processa em frames (ajusta conforme necessário)
    float detected_freq = 0.0f;
    int frames_processed = 0;
    
    for (int i = 0; i + FRAME_LEN <= total_samples; i += FRAME_LEN) {
        float freq = dsp_process(buffer + i);
        if (freq > 0) {
            detected_freq = freq;
            frames_processed++;
        }
    }
    
    free(buffer);
    
    return detected_freq;
}

// Grupo de testes
TEST_GROUP(DSPTests) {
    void setup() {
    }
    
    void teardown() {
    }
};

TEST(DSPTests, Test_A_109_68hz_neck) {
    const char* filename = "samples/4000_sample_rate/A-109_68hz-neck.wav";
    float expected = 109.68f;
    float detected = test_dsp_process_with_wav(filename);
    
    printf("\nTest: %s\n", filename);
    printf("Expected: %.1f Hz, Detected: %.1f Hz\n", expected, detected);
    
    DOUBLES_EQUAL(expected, detected, FREQUENCY_TOLERANCE);
}

TEST(DSPTests, Test_B_246_28hz_neck) {
    const char* filename = "samples/4000_sample_rate/B-246_28hz-neck.wav";
    float expected = 246.28f; // B3 = 246.94 Hz
    float detected = test_dsp_process_with_wav(filename);
    
    printf("\nTest: %s\n", filename);
    printf("Expected: %.1f Hz, Detected: %.1f Hz\n", expected, detected);
    
    DOUBLES_EQUAL(expected, detected, FREQUENCY_TOLERANCE);
}

TEST(DSPTests, Test_D_146_Hz_Neck) {
    const char* filename = "samples/4000_sample_rate/D-146_02hz-neck.wav";
    float expected = 146.02f; // D3 = 146.83 Hz
    float detected = test_dsp_process_with_wav(filename);
    
    printf("\nTest: %s\n", filename);
    printf("Expected: %.1f Hz, Detected: %.1f Hz\n", expected, detected);
    
    DOUBLES_EQUAL(expected, detected, FREQUENCY_TOLERANCE);
}

TEST(DSPTests, Test_E_329_05hz_neck) {
    const char* filename = "samples/4000_sample_rate/E-329_05hz-neck.wav";
    float expected = 329.05f; // E4 = 329.63 Hz
    float detected = test_dsp_process_with_wav(filename);
    
    printf("\nTest: %s\n", filename);
    printf("Expected: %.1f Hz, Detected: %.1f Hz\n", expected, detected);
    
    DOUBLES_EQUAL(expected, detected, FREQUENCY_TOLERANCE);
}

TEST(DSPTests, Test_E_82_76hz_neck) {
    const char* filename = "samples/4000_sample_rate/E-82_76hz-neck.wav";
    float expected = 82.76f; // E2 = 82.41 Hz
    float detected = test_dsp_process_with_wav(filename);
    
    printf("\nTest: %s\n", filename);
    printf("Expected: %.1f Hz, Detected: %.1f Hz\n", expected, detected);
    
    DOUBLES_EQUAL(expected, detected, FREQUENCY_TOLERANCE);
}

TEST(DSPTests, Test_G_195_14hz_neck) {
    const char* filename = "samples/4000_sample_rate/G-195_14hz-neck.wav";
    float expected = 195.14f; // G3 = 196.00 Hz
    float detected = test_dsp_process_with_wav(filename);
    
    printf("\nTest: %s\n", filename);
    printf("Expected: %.1f Hz, Detected: %.1f Hz\n", expected, detected);
    
    DOUBLES_EQUAL(expected, detected, FREQUENCY_TOLERANCE);
}

TEST(DSPTests, Test_Performance) {
    const char* filename = "samples/4000_sample_rate/G-195_14hz-neck.wav";
    uint16_t* buffer = NULL;
    int total_samples = 0;
    
    load_wav_file(filename, &buffer, &total_samples);
    
    if (!buffer || total_samples == 0) {
        FAIL("Failed to load sample for performance test");
        return;
    }
    
    dsp_init();
    
    // Mede tempo de processamento
    clock_t start = clock();
    int iterations = 100;
    
    for (int iter = 0; iter < iterations; iter++) {
        for (int i = 0; i + FRAME_LEN <= total_samples; i += FRAME_LEN) {
            dsp_process(buffer + i);
        }
    }
    
    clock_t end = clock();
    double cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    printf("\nPerformance Test:\n");
    printf("  Total samples: %d\n", total_samples);
    printf("  Frame size: %d\n", FRAME_LEN);
    printf("  Iterations: %d\n", iterations);
    printf("  Total time: %.3f seconds\n", cpu_time_used);
    printf("  Time per frame: %.6f seconds\n", 
           cpu_time_used / (iterations * (total_samples / FRAME_LEN)));
    
    free(buffer);
}
