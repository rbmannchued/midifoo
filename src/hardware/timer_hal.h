#ifndef TIMER_HAL_H
#define TIMER_HAL_H

#include <stdint.h>
#include <libopencm3/stm32/timer.h>

typedef struct {
    uint32_t timer;       // ex: TIM2, TIM3
    uint32_t rcc_timer;   // ex: RCC_TIM2, RCC_TIM3...
    uint16_t prescaler;   // define base freq
    uint32_t period;      // define  overflow countdown
} timer_hal_config_t;

void timer_hal_init(const timer_hal_config_t *cfg);
void timer_hal_start(const timer_hal_config_t *cfg);
void timer_hal_stop(const timer_hal_config_t *cfg);

#endif
