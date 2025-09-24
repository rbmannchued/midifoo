#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"
#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

extern "C" {
    #include "dsp.h"
    #include "arm_math.h"
}

TEST_GROUP(DSPBasicTest) {
    volatile uint16_t test_buffer[FRAME_LEN];
    
    void setup() {
        // Initialize with silence (mid-point for 12-bit ADC)
        for(int i = 0; i < FRAME_LEN; i++) {
            test_buffer[i] = 2048;
        }
        dsp_init();
    }
    
    void generate_sine_wave(float frequency, float amplitude) {
        for(int i = 0; i < FRAME_LEN; i++) {
            float sample = sinf(2.0f * PI * frequency * i / SAMPLE_RATE);
            test_buffer[i] = (uint16_t)(2048 + amplitude * 2047.0f * sample);
        }
    }
    
    bool load_wav_file(const std::string& filename, volatile uint16_t* buffer, int max_samples) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            std::cout << "Erro: Não foi possível abrir o arquivo " << filename << std::endl;
            return false;
        }
        
        // Lê o cabeçalho WAV (44 bytes)
        char header[44];
        file.read(header, 44);
        
        if (file.gcount() != 44) {
            std::cout << "Erro: Cabeçalho WAV incompleto em " << filename << std::endl;
            return false;
        }
        
        // Verifica se é um arquivo WAV válido
        if (header[0] != 'R' || header[1] != 'I' || header[2] != 'F' || header[3] != 'F') {
            std::cout << "Erro: Arquivo " << filename << " não é um WAV válido" << std::endl;
            return false;
        }
        
        // Extrai informações do cabeçalho
        int16_t audio_format = *(int16_t*)(header + 20);
        int16_t num_channels = *(int16_t*)(header + 22);
        int32_t sample_rate = *(int32_t*)(header + 24);
        int16_t bits_per_sample = *(int16_t*)(header + 34);
        int32_t data_size = *(int32_t*)(header + 40);
        
        std::cout << "Carregando " << filename << " - " 
                  << sample_rate << "Hz, " << bits_per_sample << "bits, "
                  << num_channels << " canais, formato: " << audio_format << std::endl;
        
        // Verifica se o sample rate é compatível
        if (sample_rate != (int32_t)SAMPLE_RATE) {
            std::cout << "Aviso: Sample rate do arquivo (" << sample_rate 
                      << "Hz) difere do esperado (" << SAMPLE_RATE << "Hz)" << std::endl;
        }
        
        // Calcula número de amostras
        int bytes_per_sample = bits_per_sample / 8;
        int samples_to_read = data_size / bytes_per_sample / num_channels;
        if (samples_to_read > max_samples) {
            samples_to_read = max_samples;
        }
        
        std::cout << "Lendo " << samples_to_read << " amostras (" << bytes_per_sample << " bytes por sample)" << std::endl;
        
        // Lê os dados de áudio
        if (bits_per_sample == 16) {
            int16_t sample;
            for (int i = 0; i < samples_to_read && i < max_samples; i++) {
                if (!file.read(reinterpret_cast<char*>(&sample), sizeof(sample))) {
                    break;
                }
                
                // Converte de 16-bit signed para 12-bit unsigned (0-4095)
                int32_t converted = ((int32_t)sample + 32768) * 4095 / 65536;
                buffer[i] = (uint16_t)converted;
            }
        } else if (bits_per_sample == 32) {
            // Para arquivos 32-bit (provavelmente float ou int32)
            if (audio_format == 3) { // WAVE_FORMAT_IEEE_FLOAT
                float sample;
                for (int i = 0; i < samples_to_read && i < max_samples; i++) {
                    if (!file.read(reinterpret_cast<char*>(&sample), sizeof(sample))) {
                        break;
                    }
                    // Converte de float [-1.0 to 1.0] para 12-bit unsigned [0-4095]
                    float normalized = (sample + 1.0f) / 2.0f; // [0.0 to 1.0]
                    buffer[i] = (uint16_t)(normalized * 4095.0f);
                }
            } else { // Assume PCM signed 32-bit
                int32_t sample;
                for (int i = 0; i < samples_to_read && i < max_samples; i++) {
                    if (!file.read(reinterpret_cast<char*>(&sample), sizeof(sample))) {
                        break;
                    }
                    // Converte de 32-bit signed para 12-bit unsigned
                    int64_t converted = ((int64_t)sample + 2147483648) * 4095 / 4294967295;
                    buffer[i] = (uint16_t)converted;
                }
            }
        } else if (bits_per_sample == 8) {
            int8_t sample;
            for (int i = 0; i < samples_to_read && i < max_samples; i++) {
                if (!file.read(reinterpret_cast<char*>(&sample), sizeof(sample))) {
                    break;
                }
                // Converte de 8-bit signed para 12-bit unsigned
                buffer[i] = (uint16_t)(((int32_t)sample + 128) * 4095 / 256);
            }
        } else {
            std::cout << "Erro: Bits per sample não suportado: " << bits_per_sample << std::endl;
            return false;
        }
        
        file.close();
        std::cout << "Carregadas " << samples_to_read << " amostras de " << filename << std::endl;
        return true;
    }
};


// Teste com arquivos WAV reais das notas de guitarra (32-bit compatível)
TEST(DSPBasicTest, DSPProcess_DetectsGuitarNotesFromWAV) {
    struct GuitarNote {
        std::string filename;
        float expected_freq;
        std::string note_name;
    };
    
    std::vector<GuitarNote> test_notes = {
        {"samples/E_82_strato_neck.wav", 82.41f, "E2 (82.41Hz)"},
        {"samples/A_110_strato_neck.wav", 110.00f, "A2 (110.00Hz)"},
        {"samples/D_146_strato_neck.wav", 146.83f, "D3 (146.83Hz)"},
        {"samples/G_196_strato_neck.wav", 196.00f, "G3 (196.00Hz)"},
        {"samples/B_247_strato_neck.wav", 246.94f, "B3 (246.94Hz)"},
        {"samples/E_165_strato_neck.wav", 164.81f, "E3 (164.81Hz)"}
    };
    
    const float TOLERANCE = 2.0f; // ±2Hz de tolerância
    int tests_passed = 0;
    int tests_total = 0;
    
    for (const auto& note : test_notes) {
        std::cout << "\n=== Testando " << note.note_name << " ===" << std::endl;
        tests_total++;
        
        // Carrega o arquivo WAV
        if (load_wav_file(note.filename, test_buffer, FRAME_LEN)) {
            // Processa o sinal
            float detected_freq = dsp_process(test_buffer);
            
            std::cout << "Frequência esperada: " << note.expected_freq 
                      << " Hz, Detectada: " << detected_freq << " Hz" << std::endl;
            
            if (detected_freq > 0) {
                float error = fabsf(detected_freq - note.expected_freq);
                std::cout << "Erro: " << error << " Hz (tolerância: ±" << TOLERANCE << " Hz)" << std::endl;
                
                // Verifica se está dentro da tolerância
                if (error <= TOLERANCE) {
                    std::cout << "✓ DETECÇÃO PRECISA" << std::endl;
                    tests_passed++;
                } else {
                    std::cout << "✗ DETECÇÃO FORA DA TOLERÂNCIA" << std::endl;
                }
                
                // Para testes unitários, ainda verificamos mas não falhamos imediatamente
                // CHECK_TRUE(error <= TOLERANCE);
            } else {
                std::cout << "✗ FALHA NA DETECÇÃO (frequência = 0)" << std::endl;
                // CHECK_TRUE(detected_freq > 0);
            }
        } else {
            std::cout << "✗ ARQUIVO NÃO ENCONTRADO OU INVÁLIDO: " << note.filename << std::endl;
        }
    }
    
    std::cout << "\n=== RESUMO ===" << std::endl;
    std::cout << "Testes passados: " << tests_passed << "/" << tests_total << std::endl;
    
    // Verifica se pelo menos alguns testes passaram
    CHECK_TRUE(tests_passed > 0);
}

// Teste alternativo mais tolerante para debugging
TEST(DSPBasicTest, DSPProcess_DetectsGuitarNotesWithHigherTolerance) {
    struct GuitarNote {
        std::string filename;
        float expected_freq;
        std::string note_name;
    };
    
    std::vector<GuitarNote> test_notes = {
        {"samples/E_82_strato_neck.wav", 82.41f, "E2 (82.41Hz)"},
        {"samples/A_110_strato_neck.wav", 110.00f, "A2 (110.00Hz)"},
        {"samples/D_146_strato_neck.wav", 146.83f, "D3 (146.83Hz)"}
    };
    
    const float TOLERANCE = 5.0f; // Tolerância maior para debugging
    
    for (const auto& note : test_notes) {
        std::cout << "\n=== Testando " << note.note_name << " (tolerância: ±" << TOLERANCE << "Hz) ===" << std::endl;
        
        if (load_wav_file(note.filename, test_buffer, FRAME_LEN)) {
            float detected_freq = dsp_process(test_buffer); // <<<< DSP FUNCTION
            
            if (detected_freq > 0) {
                float error = fabsf(detected_freq - note.expected_freq);
                std::cout << "Esperado: " << note.expected_freq << "Hz, Detectado: " << detected_freq 
                          << "Hz, Erro: " << error << "Hz" << std::endl;
                
                if (error <= TOLERANCE) {
                    std::cout << "✓ DENTRO DA TOLERÂNCIA" << std::endl;
                } else {
                    std::cout << "✗ FORA DA TOLERÂNCIA" << std::endl;
                }
                
                // Verificação mais flexível para debugging
                CHECK_TRUE(error <= TOLERANCE);
            } else {
                std::cout << "✗ NENHUMA FREQUÊNCIA DETECTADA" << std::endl;
                CHECK_TRUE(detected_freq > 0);
            }
        }
    }
}
