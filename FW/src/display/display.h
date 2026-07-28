#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// TFT Display Driver — SPI, supports ILI9341 and ST7789
// ============================================================================

typedef enum {
    DISPLAY_ILI9341,
    DISPLAY_ST7789,
    DISPLAY_UNKNOWN,
} display_type_t;

// RGB565 color helpers
#define RGB565(r, g, b) ((uint16_t)(((r) & 0xF8) << 8 | ((g) & 0xFC) << 3 | ((b) & 0xF8) >> 3))

// Common colors
#define COLOR_BLACK     0x0000
#define COLOR_WHITE     0xFFFF
#define COLOR_RED       0xF800
#define COLOR_GREEN     0x07E0
#define COLOR_BLUE      0x001F
#define COLOR_YELLOW    0xFFE0
#define COLOR_CYAN      0x07FF
#define COLOR_ORANGE    0xFD20
#define COLOR_GRAY      0x8410
#define COLOR_DARKGRAY  0x4208

// Initialize SPI and detect/configure display
display_type_t display_init(void);

// Backlight control (0–100%)
void display_set_backlight(uint8_t percent);

// Drawing primitives
void display_fill(uint16_t color);
void display_pixel(int16_t x, int16_t y, uint16_t color);
void display_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void display_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void display_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void display_hline(int16_t x, int16_t y, int16_t w, uint16_t color);
void display_vline(int16_t x, int16_t y, int16_t h, uint16_t color);
void display_circle(int16_t cx, int16_t cy, int16_t r, uint16_t color);
void display_fill_circle(int16_t cx, int16_t cy, int16_t r, uint16_t color);

// Text rendering (embedded bitmap font)
void display_char(int16_t x, int16_t y, char c, uint16_t color, uint16_t bg, uint8_t size);
void display_text(int16_t x, int16_t y, const char *str, uint16_t color, uint16_t bg, uint8_t size);
int display_text_width(const char *str, uint8_t size);

// Raw pixel row write (for map viewport streaming)
void display_set_window(int16_t x, int16_t y, int16_t w, int16_t h);
void display_write_pixels(const uint16_t *data, uint32_t count);

// Low-level SPI (used by SD card sharing)
void display_spi_acquire(void);
void display_spi_release(void);

#ifdef __cplusplus
}
#endif

#endif // DISPLAY_H
