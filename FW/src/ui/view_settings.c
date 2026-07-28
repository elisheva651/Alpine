#include "ui.h"
#include "config.h"
#include "display/display.h"

#include <stdio.h>

// ============================================================================
// Settings View — Configuration menu
// ============================================================================

typedef enum {
    SETTING_BRIGHTNESS,
    SETTING_SLEEP_TIMEOUT,
    SETTING_UNITS,
    SETTING_GPS_RATE,
    SETTING_TRACK_INTERVAL,
    SETTING_COUNT,
} setting_item_t;

static int s_selected = 0;

// Current settings values
static uint8_t s_brightness = BL_DEFAULT_PCT;
static int s_sleep_timeout_idx = 1; // Index into timeout options
static int s_units = 0;             // 0=metric, 1=imperial
static int s_gps_rate_idx = 0;      // Index into rate options
static int s_track_interval_idx = 1; // Index into interval options

static const int sleep_timeouts[] = {15, 30, 60, 0}; // 0 = never
static const char *sleep_timeout_labels[] = {"15s", "30s", "60s", "never"};
static const int track_intervals[] = {1, 5, 10};
static const char *track_interval_labels[] = {"1s", "5s", "10s"};

void view_settings_render(void) {
    char line[40];
    int y = 8;
    const int x = 8;

    display_text(x, y, "SETTINGS", COLOR_WHITE, COLOR_BLACK, 2);
    y += 32;

    // Menu items
    const struct {
        const char *label;
        char value[16];
    } items[] = {
        {"Brightness", ""},
        {"Sleep timeout", ""},
        {"Units", ""},
        {"GPS rate", ""},
        {"Track interval", ""},
    };

    for (int i = 0; i < SETTING_COUNT; i++) {
        uint16_t color = (i == s_selected) ? COLOR_YELLOW : COLOR_WHITE;
        uint16_t bg = (i == s_selected) ? COLOR_DARKGRAY : COLOR_BLACK;

        // Draw selection highlight
        if (i == s_selected) {
            display_fill_rect(0, y - 2, SCREEN_WIDTH, 20, COLOR_DARKGRAY);
        }

        // Label
        const char *label;
        char val[20];

        switch (i) {
        case SETTING_BRIGHTNESS:
            label = "Brightness";
            snprintf(val, sizeof(val), "%d%%", s_brightness);
            break;
        case SETTING_SLEEP_TIMEOUT:
            label = "Sleep";
            snprintf(val, sizeof(val), "%s", sleep_timeout_labels[s_sleep_timeout_idx]);
            break;
        case SETTING_UNITS:
            label = "Units";
            snprintf(val, sizeof(val), "%s", s_units ? "imperial" : "metric");
            break;
        case SETTING_GPS_RATE:
            label = "GPS rate";
            snprintf(val, sizeof(val), "%dHz", s_gps_rate_idx + 1);
            break;
        case SETTING_TRACK_INTERVAL:
            label = "Track log";
            snprintf(val, sizeof(val), "%s", track_interval_labels[s_track_interval_idx]);
            break;
        default:
            label = "?";
            val[0] = '\0';
            break;
        }

        display_text(x, y, label, color, bg, 2);
        display_text(SCREEN_WIDTH - 80, y, val, color, bg, 2);
        y += 24;
    }

    // Navigation hint
    display_text(x, SCREEN_HEIGHT - 20, "UP/DOWN: select  L/R: change",
                 COLOR_GRAY, COLOR_BLACK, 1);
}

void view_settings_input(button_event_t event) {
    if (event.type != BTN_EVENT_PRESS) return;

    switch (event.button) {
    case BTN_UP:
        if (s_selected > 0) s_selected--;
        break;
    case BTN_DOWN:
        if (s_selected < SETTING_COUNT - 1) s_selected++;
        break;
    case BTN_LEFT:
    case BTN_RIGHT: {
        int dir = (event.button == BTN_RIGHT) ? 1 : -1;

        switch (s_selected) {
        case SETTING_BRIGHTNESS:
            s_brightness = (uint8_t)((int)s_brightness + dir * 10);
            if (s_brightness > 100) s_brightness = (dir > 0) ? 100 : 0;
            display_set_backlight(s_brightness);
            break;
        case SETTING_SLEEP_TIMEOUT:
            s_sleep_timeout_idx = (s_sleep_timeout_idx + dir + 4) % 4;
            break;
        case SETTING_UNITS:
            s_units = !s_units;
            break;
        case SETTING_GPS_RATE:
            s_gps_rate_idx = (s_gps_rate_idx + dir + 5) % 5;
            break;
        case SETTING_TRACK_INTERVAL:
            s_track_interval_idx = (s_track_interval_idx + dir + 3) % 3;
            break;
        }
        break;
    }
    default:
        break;
    }
}
