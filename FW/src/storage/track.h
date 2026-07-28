#ifndef TRACK_H
#define TRACK_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Track Recorder — GPX file logging + stats
// ============================================================================

bool track_start(void);
void track_stop(void);
bool track_is_recording(void);

void track_log_point(float lat, float lon, float alt, uint16_t year,
                     uint8_t month, uint8_t day,
                     uint8_t hour, uint8_t minute, uint8_t second);

float track_get_distance_m(void);
float track_get_ascent_m(void);
float track_get_descent_m(void);
uint32_t track_get_elapsed_s(void);
float track_get_avg_speed_kmh(void);
uint32_t track_get_point_count(void);

#ifdef __cplusplus
}
#endif

#endif // TRACK_H
