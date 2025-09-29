#include "timer_hal.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

void timer_hal_init(const timer_hal_config_t *cfg) {

    rcc_periph_clock_enable(cfg->rcc_timer);

    timer_set_prescaler(cfg->timer, cfg->prescaler);
    timer_set_period(cfg->timer, cfg->period);

    timer_generate_event(cfg->timer, TIM_EGR_UG);
    timer_clear_flag(cfg->timer, TIM_EGR_UG);

    timer_set_master_mode(cfg->timer, TIM_CR2_MMS_UPDATE);
}

void timer_hal_start(const timer_hal_config_t *cfg) {
    timer_enable_counter(cfg->timer);
}

void timer_hal_stop(const timer_hal_config_t *cfg) {
    timer_disable_counter(cfg->timer);
}
