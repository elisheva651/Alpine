#include "ui.h"
#include "config.h"
#include "shared.h"
#include "display/display.h"
#include "storage/map_loader.h"

#include <stdio.h>

// ============================================================================
// Map View — Geo-referenced map with GPS position overlay
// ============================================================================

#define PAN_STEP 40  // Pixels per d-pad press

static map_t s_map;
static bool s_map_loaded = false;
static int16_t s_viewport_x = 0;
static int16_t s_viewport_y = 0;

static void center_on_gps(void) {
    SHARED_LOCK();
    float lat = g_shared.latitude;
    float lon = g_shared.longitude;
    SHARED_UNLOCK();

    int16_t px, py;
    if (map_gps_to_pixel(&s_map, lat, lon, &px, &py)) {
        s_viewport_x = px - SCREEN_WIDTH / 2;
        s_viewport_y = py - SCREEN_HEIGHT / 2;
    }
}

static void try_load_map(void) {
    if (s_map_loaded) return;

    // Try to load the first available map
    char map_names[4][64];
    int count = map_list(map_names, 4);
    if (count > 0) {
        char path[128];
        snprintf(path, sizeof(path), "%s/%s", MAP_DIR, map_names[0]);
        if (map_load(path, &s_map)) {
            s_map_loaded = true;
            center_on_gps();
        }
    }
}

void view_map_render(void) {
    try_load_map();

    if (!s_map_loaded) {
        display_fill(COLOR_BLACK);
        display_text(20, 140, "No maps on SD", COLOR_WHITE, COLOR_BLACK, 2);
        display_text(20, 170, "Upload via USB", COLOR_GRAY, COLOR_BLACK, 1);
        return;
    }

    // Render map viewport (streams from SD)
    map_render_viewport(&s_map, s_viewport_x, s_viewport_y);

    // Overlay GPS position
    SHARED_LOCK();
    float lat = g_shared.latitude;
    float lon = g_shared.longitude;
    float heading = g_shared.heading_valid ? g_shared.heading : g_shared.course;
    float alt = g_shared.baro_valid ? g_shared.altitude_baro : g_shared.altitude_gps;
    gps_fix_t fix = g_shared.fix;
    SHARED_UNLOCK();

    if (fix >= GPS_FIX_2D) {
        int16_t px, py;
        if (map_gps_to_pixel(&s_map, lat, lon, &px, &py)) {
            // Convert to screen coordinates
            int16_t sx = px - s_viewport_x;
            int16_t sy = py - s_viewport_y;

            if (sx >= 0 && sx < SCREEN_WIDTH && sy >= 0 && sy < SCREEN_HEIGHT) {
                // Draw position dot
                display_fill_circle(sx, sy, 5, COLOR_RED);
                display_circle(sx, sy, 6, COLOR_WHITE);
            }
        }
    }

    // Bottom bar
    display_fill_rect(0, SCREEN_HEIGHT - 20, SCREEN_WIDTH, 20, COLOR_BLACK);
    char info[40];
    snprintf(info, sizeof(info), "ALT: %.0fm", alt);
    display_text(4, SCREEN_HEIGHT - 16, info, COLOR_WHITE, COLOR_BLACK, 1);
}

void view_map_input(button_event_t event) {
    if (event.type != BTN_EVENT_PRESS) return;

    switch (event.button) {
    case BTN_UP:     s_viewport_y -= PAN_STEP; break;
    case BTN_DOWN:   s_viewport_y += PAN_STEP; break;
    case BTN_LEFT:   s_viewport_x -= PAN_STEP; break;
    case BTN_RIGHT:  s_viewport_x += PAN_STEP; break;
    case BTN_SELECT: center_on_gps();           break;
    default: break;
    }
}
