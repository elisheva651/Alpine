#ifndef GPS_H
#define GPS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// GPS Module — UART + NMEA parsing via minmea
// ============================================================================

// Initialize UART for GPS module
void gps_init(void);

// Process incoming UART data (call from core1 loop)
// Parses NMEA sentences and updates shared GPS data
void gps_poll(void);

// Check if GPS has a valid fix
bool gps_has_fix(void);

#ifdef __cplusplus
}
#endif

#endif // GPS_H
