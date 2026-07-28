#ifndef POWER_H
#define POWER_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Power Management — Battery, sleep/wake, backlight
// ============================================================================

// Initialize ADC for battery reading
void power_init(void);

// Read battery voltage (returns volts, e.g. 3.7)
float power_get_battery_voltage(void);

// Get battery percentage (0–100)
uint8_t power_get_battery_percent(void);

// Check battery thresholds
bool power_is_low_battery(void);       // Below BAT_VOLTAGE_WARN
bool power_is_critical_battery(void);  // Below BAT_VOLTAGE_CRIT

// Enter deep sleep (dormant mode, wake on button press)
void power_enter_sleep(void);

// Screen-off mode (GPS stays active for tracking)
void power_screen_off(void);
void power_screen_on(void);
bool power_is_screen_on(void);

#ifdef __cplusplus
}
#endif

#endif // POWER_H
