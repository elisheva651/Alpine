#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// Alpine GPS — Pin Definitions & Constants
// ============================================================================

// --- UART: GPS Module ---
#define GPS_UART_ID     uart0
#define GPS_TX_PIN      0   // GP0
#define GPS_RX_PIN      1   // GP1
#define GPS_BAUD_RATE   9600

// --- SPI0: Shared bus (TFT + SD Card) ---
#define SPI_PORT        spi0
#define SPI_SCK_PIN     2   // GP2
#define SPI_MOSI_PIN    3   // GP3
#define SPI_MISO_PIN    4   // GP4

// --- TFT Display ---
#define TFT_CS_PIN      5   // GP5
#define TFT_DC_PIN      7   // GP7
#define TFT_RST_PIN     8   // GP8
#define TFT_BL_PIN      9   // GP9 (PWM backlight)
#define TFT_SPI_FREQ    (40 * 1000 * 1000)  // 40 MHz for display

#define SCREEN_WIDTH    240
#define SCREEN_HEIGHT   320

// --- SD Card ---
#define SD_CS_PIN       6   // GP6
#define SD_SPI_FREQ     (25 * 1000 * 1000)  // 25 MHz for SD

// --- Buttons (active low, internal pull-up) ---
#define BTN_UP_PIN      10  // GP10
#define BTN_DOWN_PIN    11  // GP11
#define BTN_LEFT_PIN    12  // GP12
#define BTN_RIGHT_PIN   13  // GP13
#define BTN_SELECT_PIN  14  // GP14

#define BTN_DEBOUNCE_MS     30
#define BTN_LONG_PRESS_MS   3000  // 3s hold for power off

// --- Battery ADC ---
#define BAT_ADC_PIN     26  // GP26 (ADC0)
#define BAT_ADC_INPUT   0   // ADC input 0
// Voltage divider: 100K/100K → ADC reads half of battery voltage
// ADC reference: 3.3V, 12-bit (0–4095)
// V_bat = ADC_raw * 3.3 / 4095 * 2
#define BAT_DIVIDER_RATIO   2.0f
#define BAT_VOLTAGE_WARN    3.5f    // Low battery warning
#define BAT_VOLTAGE_CRIT    3.4f    // Forced sleep
#define BAT_VOLTAGE_FULL    4.2f    // Fully charged
#define BAT_VOLTAGE_EMPTY   3.3f    // Considered empty

// --- Backlight PWM ---
#define BL_PWM_FREQ     1000    // 1 kHz
#define BL_DEFAULT_PCT  80      // Default brightness %

// --- Power / Sleep ---
#define SLEEP_SCREEN_OFF_S  30  // Screen off after 30s inactivity
#define SLEEP_DEEP_S        300 // Deep sleep after 5 min (if not tracking)

// --- Track Recording ---
#define TRACK_DEFAULT_INTERVAL_S    5   // Log every 5 seconds
#define TRACK_DIR       "/tracks"
#define MAP_DIR         "/maps"

// --- Map Viewport ---
#define MAP_ROW_BUF_SIZE    (SCREEN_WIDTH * 2)  // 480 bytes per row (RGB565)
#define MAP_NUM_ROW_BUFS    2                    // Double-buffer

#endif // CONFIG_H
