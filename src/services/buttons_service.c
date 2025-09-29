#include "buttons_service.h"
#include "buttons_hal.h"

static const Button *buttons;
static uint8_t button_count;

void buttons_service_init(const Button *btn_array, uint8_t num_buttons) {
    buttons = btn_array;
    button_count = num_buttons;

    //passa os pinos para a buttons_hal
    static uint16_t pins[16];
    for (uint8_t i = 0; i < num_buttons; i++) {
        pins[i] = buttons[i].pin;
    }
    buttons_hal_init(pins, num_buttons);
}

void buttons_poll_task(void *args) {
    (void) args;
    bool state[16] = {0};
    TickType_t press_time[16] = {0};

    for (;;) {
        for (uint8_t i = 0; i < button_count; i++) {
            bool pressed = buttons_hal_read(i);

            if (pressed != state[i]) {
                vTaskDelay(pdMS_TO_TICKS(20)); // debounce
                pressed = buttons_hal_read(i);
                if (pressed != state[i]) {
                    state[i] = pressed;
                    ButtonEvent ev = { i, buttons[i].user_ctx };

                    if (pressed) {
                        press_time[i] = xTaskGetTickCount();
                        if (buttons[i].callback)
                            buttons[i].callback(BUTTON_EVENT_PRESSED, &ev);
                    } else {
                        TickType_t held_ticks = xTaskGetTickCount() - press_time[i];
                        uint32_t held_ms = held_ticks * portTICK_PERIOD_MS;
                        if (buttons[i].callback) {
                            if (held_ms >= 1000)
                                buttons[i].callback(BUTTON_EVENT_LONGPRESS, &ev);
                            else
                                buttons[i].callback(BUTTON_EVENT_RELEASED, &ev);
                        }
                    }
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
