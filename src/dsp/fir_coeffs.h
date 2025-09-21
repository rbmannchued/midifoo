#ifndef FIR_COEFFS_H
#define FIR_COEFFS_H


#include <arm_math.h>
#include "dsp.h"

#define NUM_TAPS 64

extern const float32_t fir_coeffs[NUM_TAPS];
#endif
