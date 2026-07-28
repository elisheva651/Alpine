#ifndef COMPASS_H
#define COMPASS_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Compass (Magnetometer) — via u-blox M10 module
// ============================================================================

void compass_init(void);

// Poll magnetometer data (call from core1 loop)
void compass_poll(void);

// Get current heading in degrees (0–360, 0=N, 90=E)
float compass_get_heading(void);
bool compass_is_valid(void);

#ifdef __cplusplus
}
#endif

#endif // COMPASS_H
