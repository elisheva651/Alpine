#include "ui.h"
#include "config.h"
#include "shared.h"
#include "display/display.h"
#include "storage/track.h"

#include <stdio.h>

// ============================================================================
// Track View — Recording stats + breadcrumb trail
// ============================================================================

void view_track_render(void) {
    char line[40];
    int y = 8;
    const int x = 8;

    bool recording = track_is_recording();

    // Header
    if (recording) {
        display_text(x, y, "TRACK RECORDING", COLOR_RED, COLOR_BLACK, 2);
        display_fill_circle(SCREEN_WIDTH - 20, y + 7, 6, COLOR_RED); // Red dot
    } else {
        display_text(x, y, "TRACK STOPPED", COLOR_GRAY, COLOR_BLACK, 2);
    }
    y += 32;

    // Stats
    SHARED_LOCK();
    float dist = g_shared.track_distance_m;
    float ascent = g_shared.track_ascent_m;
    float descent = g_shared.track_descent_m;
    SHARED_UNLOCK();

    uint32_t elapsed = track_get_elapsed_s();
    float avg_spd = track_get_avg_speed_kmh();

    // Distance
    if (dist >= 1000.0f) {
        snprintf(line, sizeof(line), "Dist:  %.1f km", dist / 1000.0f);
    } else {
        snprintf(line, sizeof(line), "Dist:  %.0f m", dist);
    }
    display_text(x, y, line, COLOR_WHITE, COLOR_BLACK, 2);
    y += 24;

    // Time
    uint32_t hours = elapsed / 3600;
    uint32_t mins = (elapsed % 3600) / 60;
    snprintf(line, sizeof(line), "Time:  %luh %02lum", hours, mins);
    display_text(x, y, line, COLOR_WHITE, COLOR_BLACK, 2);
    y += 24;

    // Ascent
    snprintf(line, sizeof(line), "Ascent: +%.0fm", ascent);
    display_text(x, y, line, COLOR_GREEN, COLOR_BLACK, 2);
    y += 24;

    // Descent
    snprintf(line, sizeof(line), "Descent: -%.0fm", descent);
    display_text(x, y, line, COLOR_ORANGE, COLOR_BLACK, 2);
    y += 24;

    // Average speed
    snprintf(line, sizeof(line), "Avg spd: %.1f km/h", avg_spd);
    display_text(x, y, line, COLOR_WHITE, COLOR_BLACK, 2);
    y += 36;

    // Instructions
    if (recording) {
        display_text(x, SCREEN_HEIGHT - 20, "SELECT: stop", COLOR_GRAY, COLOR_BLACK, 1);
    } else {
        display_text(x, SCREEN_HEIGHT - 20, "SELECT: start recording", COLOR_GRAY, COLOR_BLACK, 1);
    }
}

void view_track_input(button_event_t event) {
    if (event.type != BTN_EVENT_PRESS) return;

    // Only respond on the Track view's own select (view cycling handled by ui.c)
    // Note: Select press in track view toggles recording instead of switching views.
    // This is handled specially — we re-purpose d-pad buttons if needed.

    // Up/Down could scroll through saved tracks in the future
    (void)event;
}
