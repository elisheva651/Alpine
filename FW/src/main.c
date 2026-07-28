#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/watchdog.h"

#include "config.h"
#include "shared.h"
#include "display/display.h"
#include "input/buttons.h"
#include "storage/sd.h"
#include "power/power.h"
#include "ui/ui.h"
#include "usb/msc.h"

// Core1 entry point (defined in core1.c)
extern void core1_entry(void);

// Global shared data
shared_data_t g_shared = {0};
mutex_t g_shared_mutex;

void setup(void) {
    // Initialize shared data mutex
    mutex_init(&g_shared_mutex);

    // Initialize subsystems
    power_init();
    display_init();
    buttons_init();

    if (sd_init()) {
        sd_mount();
    }

    // Launch core1 (GPS + sensors + track logging)
    multicore_launch_core1(core1_entry);

    // Initialize UI (after display is ready)
    ui_init();
}

void loop(void) {
    // Check for USB connection -> enter mass storage mode
    if (msc_is_connected()) {
        msc_run();
        // After USB disconnect, reboot
        watchdog_reboot(0, 0, 0);
    }

    // Poll buttons
    button_event_t event = buttons_poll();
    if (event.type != BTN_EVENT_NONE) {
        // Long-press Select = power off
        if (event.button == BTN_SELECT && event.type == BTN_EVENT_LONG_PRESS) {
            power_enter_sleep();
            return;
        }
        ui_handle_input(event);
    }

    // Check battery
    if (power_is_critical_battery()) {
        power_enter_sleep();
    }

    // Render UI
    if (power_is_screen_on()) {
        ui_render();
    }

    // Sleep timeout
    uint32_t idle_ms = to_ms_since_boot(get_absolute_time()) - buttons_last_activity_ms();
    if (power_is_screen_on() && idle_ms > (SLEEP_SCREEN_OFF_S * 1000)) {
        power_screen_off();
    }
    if (!power_is_screen_on() && idle_ms > (SLEEP_DEEP_S * 1000)) {
        // Don't deep sleep if track recording is active
        SHARED_LOCK();
        bool recording = g_shared.track_recording;
        SHARED_UNLOCK();
        if (!recording) {
            power_enter_sleep();
        }
    }

    sleep_ms(20); // ~50 Hz UI refresh
}
