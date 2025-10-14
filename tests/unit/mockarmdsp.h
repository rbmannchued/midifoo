#ifndef MOCK_ARM_DSP_H
#define MOCK_ARM_DSP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <math.h>

// Estruturas simplificadas para teste
typedef struct {
    uint32_t fftLen;
    // ... outros campos necessários
} arm_rfft_fast_instance_f32;

typedef struct {
    uint16_t numTaps;
    float32_t *pCoeffs;
    float32_t *pState;
} arm_fir_instance_f32;

// Protótipos das funções ARM que serão mockadas
void arm_rfft_fast_init_f32(arm_rfft_fast_instance_f32 *S, uint32_t fftLen);
void arm_fir_init_f32(arm_fir_instance_f32 *S, uint16_t numTaps, float32_t *pCoeffs, float32_t *pState, uint32_t blockSize);
void arm_fir_f32(const arm_fir_instance_f32 *S, float32_t *pSrc, float32_t *pDst, uint32_t blockSize);
void arm_rfft_fast_f32(arm_rfft_fast_instance_f32 *S, float32_t *p, float32_t *pOut, uint8_t ifftFlag);
void arm_cmplx_mag_f32(float32_t *pSrc, float32_t *pDst, uint32_t numSamples);

#ifdef __cplusplus
}
#endif

#endif // MOCK_ARM_DSP_H
