#ifndef TEST_DSP_H
#define TEST_DSP_H

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "CppUTest/CommandLineTestRunner.h"

extern "C" {
    #include "dsp.h"
    #include "arm_math.h"
}

// Funções de auxílio para testes
float test_dsp_process_with_wav(const char* filename);
void load_wav_file(const char* filename, uint16_t** buffer, int* size);

#endif // TEST_DSP_H
