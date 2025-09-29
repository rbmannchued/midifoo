#ifndef BUTTONS_SERVICE_H
#define BUTTONS_SERVICE_H

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"

typedef enum {
    BUTTON_EVENT_PRESSED,
    BUTTON_EVENT_RELEASED,
    BUTTON_EVENT_LONGPRESS
} ButtonEventType;

typedef struct {
    uint8_t id;
    void *user_ctx;
} ButtonEvent;

typedef void (*button_cb_t)(ButtonEventType event, ButtonEvent *ctx);

typedef struct {
    uint16_t pin;
    button_cb_t callback;
    void *user_ctx;
} Button;

void buttons_service_init(const Button *btn_array, uint8_t num_buttons);
void buttons_poll_task(void *args);

#endif
