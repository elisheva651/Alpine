#include "track.h"
#include "sd.h"
#include "config.h"
#include "shared.h"

#include "pico/stdlib.h"
#include "ff.h"
using namespace fatfs;

#include <stdio.h>
#include <string.h>
#include <math.h>

// ============================================================================
// Track Recorder — GPX file logging + stats
// ============================================================================

static FIL s_gpx_file;
static bool s_recording = false;
static bool s_file_open = false;

// Stats
static float s_distance_m = 0.0f;
static float s_ascent_m = 0.0f;
static float s_descent_m = 0.0f;
static uint32_t s_start_time = 0;
static uint32_t s_point_count = 0;

// Previous point for distance/elevation calculations
static float s_prev_lat = 0.0f;
static float s_prev_lon = 0.0f;
static float s_prev_alt = 0.0f;
static bool s_has_prev = false;

// Haversine distance between two GPS points (meters)
static float haversine_m(float lat1, float lon1, float lat2, float lon2) {
    const float R = 6371000.0f; // Earth radius in meters
    float dlat = (lat2 - lat1) * (float)M_PI / 180.0f;
    float dlon = (lon2 - lon1) * (float)M_PI / 180.0f;
    float a = sinf(dlat / 2) * sinf(dlat / 2) +
              cosf(lat1 * (float)M_PI / 180.0f) *
              cosf(lat2 * (float)M_PI / 180.0f) *
              sinf(dlon / 2) * sinf(dlon / 2);
    float c = 2.0f * atan2f(sqrtf(a), sqrtf(1.0f - a));
    return R * c;
}

bool track_start(void) {
    if (s_recording) return true;

    // Create filename from current date
    SHARED_LOCK();
    uint16_t year = g_shared.year;
    uint8_t month = g_shared.month;
    uint8_t day = g_shared.day;
    SHARED_UNLOCK();

    char filename[64];
    snprintf(filename, sizeof(filename), "%s/%04d-%02d-%02d.gpx",
             TRACK_DIR, year, month, day);

    // Open file (append if exists)
    bool new_file = !sd_file_exists(filename);
    FRESULT res = f_open(&s_gpx_file, filename,
                         FA_WRITE | FA_OPEN_APPEND | FA_OPEN_ALWAYS);
    if (res != FR_OK) return false;

    s_file_open = true;

    if (new_file || f_size(&s_gpx_file) == 0) {
        // Write GPX header
        const char *header =
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<gpx version=\"1.1\" creator=\"Alpine GPS\">\n"
            " <trk>\n"
            "  <name>Track</name>\n"
            "  <trkseg>\n";
        UINT bw;
        f_write(&s_gpx_file, header, strlen(header), &bw);
    }

    s_recording = true;
    s_distance_m = 0.0f;
    s_ascent_m = 0.0f;
    s_descent_m = 0.0f;
    s_start_time = to_ms_since_boot(get_absolute_time());
    s_point_count = 0;
    s_has_prev = false;

    SHARED_LOCK();
    g_shared.track_recording = true;
    SHARED_UNLOCK();

    return true;
}

void track_stop(void) {
    if (!s_recording) return;

    if (s_file_open) {
        const char *footer =
            "  </trkseg>\n"
            " </trk>\n"
            "</gpx>\n";
        UINT bw;
        f_write(&s_gpx_file, footer, strlen(footer), &bw);
        f_sync(&s_gpx_file);
        f_close(&s_gpx_file);
        s_file_open = false;
    }

    s_recording = false;

    SHARED_LOCK();
    g_shared.track_recording = false;
    SHARED_UNLOCK();
}

bool track_is_recording(void) {
    return s_recording;
}

void track_log_point(float lat, float lon, float alt, uint16_t year,
                     uint8_t month, uint8_t day,
                     uint8_t hour, uint8_t minute, uint8_t second) {
    if (!s_recording || !s_file_open) return;

    // Write trackpoint in GPX format
    char buf[256];
    int len = snprintf(buf, sizeof(buf),
        "   <trkpt lat=\"%.6f\" lon=\"%.6f\">\n"
        "    <ele>%.1f</ele>\n"
        "    <time>%04d-%02d-%02dT%02d:%02d:%02dZ</time>\n"
        "   </trkpt>\n",
        lat, lon, alt, year, month, day, hour, minute, second);

    UINT bw;
    f_write(&s_gpx_file, buf, len, &bw);

    // Flush periodically (every 10 points)
    s_point_count++;
    if (s_point_count % 10 == 0) {
        f_sync(&s_gpx_file);
    }

    // Update stats
    if (s_has_prev) {
        float dist = haversine_m(s_prev_lat, s_prev_lon, lat, lon);
        s_distance_m += dist;

        float dalt = alt - s_prev_alt;
        if (dalt > 0.5f) {
            s_ascent_m += dalt;
        } else if (dalt < -0.5f) {
            s_descent_m += (-dalt);
        }
    }

    s_prev_lat = lat;
    s_prev_lon = lon;
    s_prev_alt = alt;
    s_has_prev = true;
}

float track_get_distance_m(void) { return s_distance_m; }
float track_get_ascent_m(void) { return s_ascent_m; }
float track_get_descent_m(void) { return s_descent_m; }

uint32_t track_get_elapsed_s(void) {
    if (!s_recording) return 0;
    return (to_ms_since_boot(get_absolute_time()) - s_start_time) / 1000;
}

float track_get_avg_speed_kmh(void) {
    uint32_t elapsed = track_get_elapsed_s();
    if (elapsed == 0) return 0.0f;
    return (s_distance_m / 1000.0f) / ((float)elapsed / 3600.0f);
}

uint32_t track_get_point_count(void) { return s_point_count; }
