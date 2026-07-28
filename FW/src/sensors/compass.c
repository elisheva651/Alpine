#include "compass.h"
#include "config.h"

#include "pico/stdlib.h"
#include "hardware/uart.h"

#include <math.h>

// ============================================================================
// Compass (Magnetometer) — via u-blox M10 UBX protocol
//
// The GEPRC M10 module has a built-in QMC5883L magnetometer.
// Data is accessed via UBX protocol on the same UART as GPS.
// TODO: Implement UBX binary protocol for magnetometer readings.
//       For now, heading is derived from GPS course when moving.
// ============================================================================

static float s_heading = 0.0f;
static bool s_valid = false;

void compass_init(void) {
    // Magnetometer shares UART with GPS (already initialized in gps_init)
    // TODO: Send UBX configuration to enable magnetometer output
    s_heading = 0.0f;
    s_valid = false;
}

void compass_poll(void) {
    // TODO: Parse UBX magnetometer messages
    // For now, heading comes from GPS course (set in gps.c via shared data)
    // This only works when moving; stationary heading requires magnetometer
}

float compass_get_heading(void) {
    return s_heading;
}

bool compass_is_valid(void) {
    return s_valid;
}
