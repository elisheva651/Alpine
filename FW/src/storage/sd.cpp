#include "sd.h"
#include "config.h"
#include "display/display.h"

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

#include "ff.h"
using namespace fatfs;

#include <string.h>

// ============================================================================
// SD Card — SPI driver + FatFs
//
// Shares SPI0 bus with TFT display (different CS pins).
// Must acquire/release SPI bus and adjust clock speed.
// ============================================================================

static FATFS s_fatfs;
static bool s_mounted = false;

// SD card SPI helpers
static inline void sd_cs_select(void) {
    display_spi_acquire(); // Ensure TFT CS is high
    spi_set_baudrate(SPI_PORT, SD_SPI_FREQ);
    gpio_put(SD_CS_PIN, 0);
}

static inline void sd_cs_deselect(void) {
    gpio_put(SD_CS_PIN, 1);
    display_spi_release(); // Restore TFT SPI speed
}

bool sd_init(void) {
    // CS pin for SD card
    gpio_init(SD_CS_PIN);
    gpio_set_dir(SD_CS_PIN, GPIO_OUT);
    gpio_put(SD_CS_PIN, 1); // Deselect

    // SPI bus is already initialized by display_init()
    // SD card init requires slow clock (400 kHz), then speed up

    // Send 80+ clock pulses with CS high (SD card init sequence)
    gpio_put(SD_CS_PIN, 1);
    spi_set_baudrate(SPI_PORT, 400000); // 400 kHz for init
    uint8_t dummy[10];
    memset(dummy, 0xFF, sizeof(dummy));
    spi_write_blocking(SPI_PORT, dummy, sizeof(dummy));

    spi_set_baudrate(SPI_PORT, TFT_SPI_FREQ); // Restore

    return true;
}

bool sd_mount(void) {
    FRESULT res = f_mount(&s_fatfs, "", 1); // Force mount
    if (res == FR_OK) {
        s_mounted = true;
        // Ensure required directories exist
        sd_mkdir(TRACK_DIR);
        sd_mkdir(MAP_DIR);
        return true;
    }
    s_mounted = false;
    return false;
}

void sd_unmount(void) {
    f_mount(NULL, "", 0);
    s_mounted = false;
}

bool sd_is_mounted(void) {
    return s_mounted;
}

bool sd_file_exists(const char *path) {
    FILINFO fno;
    return (f_stat(path, &fno) == FR_OK);
}

bool sd_mkdir(const char *path) {
    FRESULT res = f_mkdir(path);
    return (res == FR_OK || res == FR_EXIST);
}
