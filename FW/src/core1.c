#include "pico/stdlib.h"
#include "pico/time.h"

#include "config.h"
#include "shared.h"
#include "gps/gps.h"
#include "sensors/compass.h"
#include "sensors/baro.h"
#include "storage/track.h"

void core1_entry(void) {
    // Initialize GPS and sensors
    gps_init();
    compass_init();
    baro_init();

    uint32_t last_track_log = 0;

    while (true) {
        // Poll GPS (processes incoming UART data)
        gps_poll();

        // Poll sensors
        compass_poll();
        baro_poll();

        // Update shared data for core0
        SHARED_LOCK();

        if (compass_is_valid()) {
            g_shared.heading = compass_get_heading();
            g_shared.heading_valid = true;
        }

        if (baro_is_valid()) {
            g_shared.pressure_hpa = baro_get_pressure_hpa();
            g_shared.altitude_baro = baro_get_altitude_m();
            g_shared.temperature_c = baro_get_temperature_c();
            g_shared.baro_valid = true;
        }

        bool recording = g_shared.track_recording;
        float lat = g_shared.latitude;
        float lon = g_shared.longitude;
        float alt = g_shared.altitude_baro;
        bool has_fix = (g_shared.fix >= GPS_FIX_2D);
        uint16_t year = g_shared.year;
        uint8_t month = g_shared.month;
        uint8_t day = g_shared.day;
        uint8_t hour = g_shared.hour;
        uint8_t minute = g_shared.minute;
        uint8_t second = g_shared.second;

        SHARED_UNLOCK();

        // Track logging
        if (recording && has_fix) {
            uint32_t now = to_ms_since_boot(get_absolute_time());
            if (now - last_track_log >= (TRACK_DEFAULT_INTERVAL_S * 1000)) {
                track_log_point(lat, lon, alt, year, month, day,
                               hour, minute, second);
                last_track_log = now;

                // Update track stats in shared data
                SHARED_LOCK();
                g_shared.track_distance_m = track_get_distance_m();
                g_shared.track_ascent_m = track_get_ascent_m();
                g_shared.track_descent_m = track_get_descent_m();
                g_shared.track_points = track_get_point_count();
                SHARED_UNLOCK();
            }
        }

        sleep_ms(10); // 100 Hz poll rate
    }
}
