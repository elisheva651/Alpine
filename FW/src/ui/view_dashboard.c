#include "ui.h"
#include "config.h"
#include "shared.h"
#include "display/display.h"
#include "power/power.h"

#include <stdio.h>

// ============================================================================
// Dashboard View — Default screen showing all sensor data
//
//  ALT: 2,100m
//  HDG: 247° WSW  ↗
//  PRESS: 1013 hPa
//  SPD: 3.2 km/h
//
//  LAT: 46.4521° N
//  LON: 10.8834° E
//
//  BAT: 78%  GPS: 3D
//  12:51       5 sats
// ============================================================================

static const char *heading_to_cardinal(float heading) {
    static const char *dirs[] = {"N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
                                  "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"};
    int idx = (int)((heading + 11.25f) / 22.5f) % 16;
    return dirs[idx];
}

void view_dashboard_render(void) {
    char line[40];
    int y = 8;
    const int x = 8;
    const uint8_t sz = 2; // Font size 2x

    // Read shared data (snapshot under lock)
    SHARED_LOCK();
    shared_data_t data = g_shared;
    SHARED_UNLOCK();

    // Altitude
    float alt = data.baro_valid ? data.altitude_baro : data.altitude_gps;
    snprintf(line, sizeof(line), "ALT: %.0fm", alt);
    display_text(x, y, line, COLOR_WHITE, COLOR_BLACK, sz);
    y += 24;

    // Heading
    float hdg = data.heading_valid ? data.heading : data.course;
    snprintf(line, sizeof(line), "HDG: %.0f %s", hdg, heading_to_cardinal(hdg));
    display_text(x, y, line, COLOR_WHITE, COLOR_BLACK, sz);
    y += 24;

    // Pressure
    if (data.baro_valid) {
        snprintf(line, sizeof(line), "PRESS: %.0f hPa", data.pressure_hpa);
    } else {
        snprintf(line, sizeof(line), "PRESS: ---");
    }
    display_text(x, y, line, COLOR_WHITE, COLOR_BLACK, sz);
    y += 24;

    // Speed
    snprintf(line, sizeof(line), "SPD: %.1f km/h", data.speed_kmh);
    display_text(x, y, line, COLOR_WHITE, COLOR_BLACK, sz);
    y += 36;

    // Coordinates
    char ns = data.latitude >= 0 ? 'N' : 'S';
    char ew = data.longitude >= 0 ? 'E' : 'W';
    float abs_lat = data.latitude >= 0 ? data.latitude : -data.latitude;
    float abs_lon = data.longitude >= 0 ? data.longitude : -data.longitude;

    snprintf(line, sizeof(line), "LAT: %.4f %c", abs_lat, ns);
    display_text(x, y, line, COLOR_CYAN, COLOR_BLACK, sz);
    y += 24;

    snprintf(line, sizeof(line), "LON: %.4f %c", abs_lon, ew);
    display_text(x, y, line, COLOR_CYAN, COLOR_BLACK, sz);
    y += 36;

    // Battery
    uint8_t bat_pct = power_get_battery_percent();
    uint16_t bat_color = bat_pct > 20 ? COLOR_GREEN : COLOR_RED;
    snprintf(line, sizeof(line), "BAT: %d%%", bat_pct);
    display_text(x, y, line, bat_color, COLOR_BLACK, sz);

    // GPS fix
    const char *fix_str;
    uint16_t fix_color;
    switch (data.fix) {
    case GPS_FIX_3D:   fix_str = "3D"; fix_color = COLOR_GREEN; break;
    case GPS_FIX_2D:   fix_str = "2D"; fix_color = COLOR_YELLOW; break;
    default:           fix_str = "--"; fix_color = COLOR_RED; break;
    }
    snprintf(line, sizeof(line), "GPS: %s", fix_str);
    display_text(x + 130, y, line, fix_color, COLOR_BLACK, sz);
    y += 24;

    // Time and satellites
    if (data.time_valid) {
        snprintf(line, sizeof(line), "%02d:%02d", data.hour, data.minute);
    } else {
        snprintf(line, sizeof(line), "--:--");
    }
    display_text(x, y, line, COLOR_GRAY, COLOR_BLACK, sz);

    snprintf(line, sizeof(line), "%d sats", data.satellites);
    display_text(x + 130, y, line, COLOR_GRAY, COLOR_BLACK, sz);
}

void view_dashboard_input(button_event_t event) {
    // Dashboard has no interactive elements
    (void)event;
}
