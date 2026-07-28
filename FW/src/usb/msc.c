#include "msc.h"
#include "config.h"
#include "display/display.h"
#include "storage/sd.h"

#include "pico/stdlib.h"

// ============================================================================
// USB Mass Storage — TinyUSB MSC device
//
// When USB cable is detected (VBUS on RP2040-Zero), firmware switches to
// mass storage mode, exposing the SD card as a USB drive.
//
// TODO: Implement TinyUSB MSC callbacks (tud_msc_read10_cb, etc.)
//       These require linking with TinyUSB and implementing the
//       block device interface over SPI SD card.
// ============================================================================

void msc_init(void) {
    // TinyUSB initialization will go here
    // tud_init(BOARD_TUD_RHPORT);
}

bool msc_is_connected(void) {
    // Check VBUS presence on RP2040-Zero
    // The RP2040-Zero has USB-C with VBUS connected
    // TODO: Implement VBUS detection
    // For now, return false (USB mode disabled until implemented)
    return false;
}

void msc_run(void) {
    // Show USB connected screen
    display_fill(COLOR_BLACK);
    display_text(40, 120, "USB", COLOR_WHITE, COLOR_BLACK, 4);
    display_text(20, 170, "CONNECTED", COLOR_WHITE, COLOR_BLACK, 3);
    display_text(20, 220, "SD card accessible", COLOR_GRAY, COLOR_BLACK, 1);
    display_text(20, 240, "from computer", COLOR_GRAY, COLOR_BLACK, 1);

    // Run TinyUSB task loop until disconnected
    while (msc_is_connected()) {
        // tud_task();
        sleep_ms(10);
    }
}
