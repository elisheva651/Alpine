#include "buttons.h"
#include "config.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"

// ============================================================================
// Button Input — Debounce + press/long-press detection
// ============================================================================

typedef struct {
    uint8_t pin;
    bool last_raw;
    bool stable;
    uint32_t debounce_time;
    uint32_t press_time;
    bool long_press_fired;
} button_state_t;

static button_state_t s_buttons[] = {
    {BTN_UP_PIN,     false, false, 0, 0, false},
    {BTN_DOWN_PIN,   false, false, 0, 0, false},
    {BTN_LEFT_PIN,   false, false, 0, 0, false},
    {BTN_RIGHT_PIN,  false, false, 0, 0, false},
    {BTN_SELECT_PIN, false, false, 0, 0, false},
};

#define NUM_BUTTONS (sizeof(s_buttons) / sizeof(s_buttons[0]))

static uint32_t s_last_activity = 0;

void buttons_init(void) {
    for (int i = 0; i < NUM_BUTTONS; i++) {
        gpio_init(s_buttons[i].pin);
        gpio_set_dir(s_buttons[i].pin, GPIO_IN);
        gpio_pull_up(s_buttons[i].pin);
    }
    s_last_activity = to_ms_since_boot(get_absolute_time());
}

button_event_t buttons_poll(void) {
    uint32_t now = to_ms_since_boot(get_absolute_time());
    button_event_t event = {BTN_NONE, BTN_EVENT_NONE};

    for (int i = 0; i < NUM_BUTTONS; i++) {
        button_state_t *btn = &s_buttons[i];
        bool raw = !gpio_get(btn->pin); // Active low

        // Debounce
        if (raw != btn->last_raw) {
            btn->debounce_time = now;
            btn->last_raw = raw;
        }

        if ((now - btn->debounce_time) < BTN_DEBOUNCE_MS) {
            continue;
        }

        bool prev_stable = btn->stable;
        btn->stable = raw;

        // Button just pressed
        if (raw && !prev_stable) {
            btn->press_time = now;
            btn->long_press_fired = false;
            s_last_activity = now;
        }

        // Button held — check long press
        if (raw && !btn->long_press_fired) {
            if ((now - btn->press_time) >= BTN_LONG_PRESS_MS) {
                btn->long_press_fired = true;
                event.button = (button_id_t)(i + 1); // BTN_UP=1, etc.
                event.type = BTN_EVENT_LONG_PRESS;
                s_last_activity = now;
                return event;
            }
        }

        // Button released — short press
        if (!raw && prev_stable && !btn->long_press_fired) {
            event.button = (button_id_t)(i + 1);
            event.type = BTN_EVENT_PRESS;
            s_last_activity = now;
            return event;
        }
    }

    return event;
}

bool buttons_any_pressed(void) {
    for (int i = 0; i < NUM_BUTTONS; i++) {
        if (!gpio_get(s_buttons[i].pin)) return true;
    }
    return false;
}

uint32_t buttons_last_activity_ms(void) {
    return s_last_activity;
}
