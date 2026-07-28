#include "power.h"
#include "config.h"
#include "display/display.h"
#include "input/buttons.h"

#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"

// ============================================================================
// Power Management — Battery ADC, sleep/wake, backlight
// ============================================================================

static bool s_screen_on = true;
static uint8_t s_saved_brightness = BL_DEFAULT_PCT;

void power_init(void) {
    adc_init();
    adc_gpio_init(BAT_ADC_PIN);
    s_screen_on = true;
}

float power_get_battery_voltage(void) {
    adc_select_input(BAT_ADC_INPUT);
    uint16_t raw = adc_read(); // 12-bit: 0–4095
    // Convert: V = raw * 3.3 / 4095 * divider_ratio
    float voltage = (float)raw * 3.3f / 4095.0f * BAT_DIVIDER_RATIO;
    return voltage;
}

uint8_t power_get_battery_percent(void) {
    float v = power_get_battery_voltage();
    if (v >= BAT_VOLTAGE_FULL) return 100;
    if (v <= BAT_VOLTAGE_EMPTY) return 0;
    // Linear interpolation between empty and full
    float pct = (v - BAT_VOLTAGE_EMPTY) / (BAT_VOLTAGE_FULL - BAT_VOLTAGE_EMPTY) * 100.0f;
    return (uint8_t)pct;
}

bool power_is_low_battery(void) {
    return power_get_battery_voltage() < BAT_VOLTAGE_WARN;
}

bool power_is_critical_battery(void) {
    return power_get_battery_voltage() < BAT_VOLTAGE_CRIT;
}

void power_enter_sleep(void) {
    // Turn off display
    display_set_backlight(0);
    s_screen_on = false;

    // Configure button pins for wake interrupt
    // Use dormant mode — wakes on GPIO edge
    // For now, use a simple deep-sleep approach with WFE
    // TODO: Implement proper RP2040 dormant mode with GPIO wake

    while (!buttons_any_pressed()) {
        __wfe(); // Wait for event (low power)
    }

    // Woke up — restore
    s_screen_on = true;
    display_set_backlight(s_saved_brightness);
}

void power_screen_off(void) {
    s_screen_on = false;
    display_set_backlight(0);
}

void power_screen_on(void) {
    s_screen_on = true;
    display_set_backlight(s_saved_brightness);
}

bool power_is_screen_on(void) {
    return s_screen_on;
}
