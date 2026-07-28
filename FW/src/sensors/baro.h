#ifndef BARO_H
#define BARO_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Barometer — via u-blox M10 module
// ============================================================================

void baro_init(void);

// Poll barometer data (call from core1 loop)
void baro_poll(void);

float baro_get_pressure_hpa(void);
float baro_get_altitude_m(void);
float baro_get_temperature_c(void);
bool baro_is_valid(void);

#ifdef __cplusplus
}
#endif

#endif // BARO_H
