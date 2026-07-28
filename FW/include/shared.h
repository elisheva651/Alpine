#ifndef SHARED_H
#define SHARED_H

#include <stdbool.h>
#include <stdint.h>
#include "pico/mutex.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Shared data between Core0 (UI) and Core1 (GPS/sensors)
// Protected by spin lock
// ============================================================================

typedef enum {
    GPS_FIX_NONE = 0,
    GPS_FIX_2D   = 1,
    GPS_FIX_3D   = 2,
} gps_fix_t;

typedef struct {
    // GPS position
    float latitude;         // Degrees (positive = N)
    float longitude;        // Degrees (positive = E)
    float altitude_gps;     // Meters (from GPS)
    float speed_kmh;        // km/h
    float course;           // Degrees (true heading from GPS)

    // GPS status
    gps_fix_t fix;
    int satellites;
    uint8_t hour, minute, second;   // UTC time from GPS
    uint8_t day, month;
    uint16_t year;
    bool time_valid;

    // Compass (magnetometer)
    float heading;          // Degrees (magnetic heading, 0=N, 90=E)
    bool heading_valid;

    // Barometer
    float pressure_hpa;     // hPa (mbar)
    float altitude_baro;    // Meters (barometric altitude, more accurate)
    float temperature_c;    // Celsius
    bool baro_valid;

    // Track recording
    bool track_recording;
    float track_distance_m;     // Total distance in meters
    float track_ascent_m;       // Total ascent in meters
    float track_descent_m;      // Total descent in meters
    uint32_t track_start_time;  // ms since boot when recording started
    uint32_t track_points;      // Number of logged points
} shared_data_t;

extern shared_data_t g_shared;
extern mutex_t g_shared_mutex;

// Thread-safe access macros
#define SHARED_LOCK()   mutex_enter_blocking(&g_shared_mutex)
#define SHARED_UNLOCK() mutex_exit(&g_shared_mutex)

#ifdef __cplusplus
}
#endif

#endif // SHARED_H
