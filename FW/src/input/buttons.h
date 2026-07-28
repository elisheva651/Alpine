#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Button Input — Debounce + press/long-press detection
// ============================================================================

typedef enum {
    BTN_NONE = 0,
    BTN_UP,
    BTN_DOWN,
    BTN_LEFT,
    BTN_RIGHT,
    BTN_SELECT,
} button_id_t;

typedef enum {
    BTN_EVENT_NONE = 0,
    BTN_EVENT_PRESS,        // Short press (released before long-press threshold)
    BTN_EVENT_LONG_PRESS,   // Held for BTN_LONG_PRESS_MS
} button_event_type_t;

typedef struct {
    button_id_t button;
    button_event_type_t type;
} button_event_t;

// Initialize button GPIOs with pull-ups
void buttons_init(void);

// Poll buttons and return event (call from main loop)
// Returns BTN_EVENT_NONE if no event
button_event_t buttons_poll(void);

// Check if any button is currently pressed (for wake detection)
bool buttons_any_pressed(void);

// Register last activity time (for sleep timeout)
uint32_t buttons_last_activity_ms(void);

#ifdef __cplusplus
}
#endif

#endif // BUTTONS_H
