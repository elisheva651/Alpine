#include "baro.h"
#include "config.h"

#include "pico/stdlib.h"

#include <math.h>

// ============================================================================
// Barometer — via u-blox M10 UBX protocol
//
// The GEPRC M10 module has a built-in BMP280 barometer.
// Data is accessed via UBX protocol on the same UART as GPS.
// TODO: Implement UBX binary protocol for barometer readings.
//       For now, altitude uses GPS altitude as fallback.
// ============================================================================

static float s_pressure = 0.0f;
static float s_altitude = 0.0f;
static float s_temperature = 0.0f;
static bool s_valid = false;

// Standard atmosphere reference pressure at sea level
#define SEALEVEL_HPA 1013.25f

void baro_init(void) {
    // Barometer shares UART with GPS (already initialized in gps_init)
    // TODO: Send UBX configuration to enable barometer output
    s_pressure = SEALEVEL_HPA;
    s_altitude = 0.0f;
    s_temperature = 20.0f;
    s_valid = false;
}

void baro_poll(void) {
    // TODO: Parse UBX barometer messages
    // Altitude from pressure: h = 44330 * (1 - (P/P0)^(1/5.255))
}

float baro_get_pressure_hpa(void) {
    return s_pressure;
}

float baro_get_altitude_m(void) {
    return s_altitude;
}

float baro_get_temperature_c(void) {
    return s_temperature;
}

bool baro_is_valid(void) {
    return s_valid;
}
