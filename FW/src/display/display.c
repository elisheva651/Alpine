#include "display.h"
#include "font.h"
#include "config.h"

#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"

#include <string.h>
#include <stdlib.h>

// ============================================================================
// TFT Display Driver — ILI9341 / ST7789 over SPI
// ============================================================================

static display_type_t s_display_type = DISPLAY_UNKNOWN;
static uint s_bl_slice;
static uint s_bl_chan;

// --- Low-level SPI helpers ---

static inline void cs_select(void) {
    gpio_put(TFT_CS_PIN, 0);
}

static inline void cs_deselect(void) {
    gpio_put(TFT_CS_PIN, 1);
}

static inline void dc_command(void) {
    gpio_put(TFT_DC_PIN, 0);
}

static inline void dc_data(void) {
    gpio_put(TFT_DC_PIN, 1);
}

static void write_cmd(uint8_t cmd) {
    dc_command();
    cs_select();
    spi_write_blocking(SPI_PORT, &cmd, 1);
    cs_deselect();
}

static void write_data(const uint8_t *data, size_t len) {
    dc_data();
    cs_select();
    spi_write_blocking(SPI_PORT, data, len);
    cs_deselect();
}

static void write_data_byte(uint8_t val) {
    write_data(&val, 1);
}

static void write_cmd_data(uint8_t cmd, const uint8_t *data, size_t len) {
    write_cmd(cmd);
    if (len > 0) {
        write_data(data, len);
    }
}

// --- Display init sequences ---

static void init_ili9341(void) {
    // Software reset
    write_cmd(0x01);
    sleep_ms(150);

    // Display off
    write_cmd(0x28);

    // Power control A
    write_cmd_data(0xCB, (uint8_t[]){0x39, 0x2C, 0x00, 0x34, 0x02}, 5);
    // Power control B
    write_cmd_data(0xCF, (uint8_t[]){0x00, 0xC1, 0x30}, 3);
    // Driver timing control A
    write_cmd_data(0xE8, (uint8_t[]){0x85, 0x00, 0x78}, 3);
    // Driver timing control B
    write_cmd_data(0xEA, (uint8_t[]){0x00, 0x00}, 2);
    // Power on sequence control
    write_cmd_data(0xED, (uint8_t[]){0x64, 0x03, 0x12, 0x81}, 4);
    // Pump ratio control
    write_cmd_data(0xF7, (uint8_t[]){0x20}, 1);

    // Power control 1
    write_cmd_data(0xC0, (uint8_t[]){0x23}, 1);
    // Power control 2
    write_cmd_data(0xC1, (uint8_t[]){0x10}, 1);
    // VCOM control 1
    write_cmd_data(0xC5, (uint8_t[]){0x3E, 0x28}, 2);
    // VCOM control 2
    write_cmd_data(0xC7, (uint8_t[]){0x86}, 1);

    // Memory access control (portrait mode)
    write_cmd_data(0x36, (uint8_t[]){0x48}, 1);

    // Pixel format: 16-bit RGB565
    write_cmd_data(0x3A, (uint8_t[]){0x55}, 1);

    // Frame rate control
    write_cmd_data(0xB1, (uint8_t[]){0x00, 0x18}, 2);
    // Display function control
    write_cmd_data(0xB6, (uint8_t[]){0x08, 0x82, 0x27}, 3);

    // Sleep out
    write_cmd(0x11);
    sleep_ms(120);

    // Display on
    write_cmd(0x29);
    sleep_ms(50);
}

static void init_st7789(void) {
    // Software reset
    write_cmd(0x01);
    sleep_ms(150);

    // Sleep out
    write_cmd(0x11);
    sleep_ms(120);

    // Memory access control (portrait)
    write_cmd_data(0x36, (uint8_t[]){0x00}, 1);

    // Pixel format: 16-bit RGB565
    write_cmd_data(0x3A, (uint8_t[]){0x55}, 1);

    // Porch setting
    write_cmd_data(0xB2, (uint8_t[]){0x0C, 0x0C, 0x00, 0x33, 0x33}, 5);

    // Gate control
    write_cmd_data(0xB7, (uint8_t[]){0x35}, 1);

    // VCOM setting
    write_cmd_data(0xBB, (uint8_t[]){0x19}, 1);

    // LCM control
    write_cmd_data(0xC0, (uint8_t[]){0x2C}, 1);

    // VDV and VRH command enable
    write_cmd_data(0xC2, (uint8_t[]){0x01}, 1);

    // VRH set
    write_cmd_data(0xC3, (uint8_t[]){0x12}, 1);

    // VDV set
    write_cmd_data(0xC4, (uint8_t[]){0x20}, 1);

    // Frame rate control
    write_cmd_data(0xC6, (uint8_t[]){0x0F}, 1);

    // Power control 1
    write_cmd_data(0xD0, (uint8_t[]){0xA4, 0xA1}, 2);

    // Display inversion on (ST7789 needs this for correct colors)
    write_cmd(0x21);

    // Display on
    write_cmd(0x29);
    sleep_ms(50);
}

static display_type_t detect_display(void) {
    // Try reading ILI9341 ID (register 0xD3)
    uint8_t buf[4] = {0};
    write_cmd(0xD3);
    dc_data();
    cs_select();
    spi_read_blocking(SPI_PORT, 0, buf, 4);
    cs_deselect();

    if (buf[2] == 0x93 && buf[3] == 0x41) {
        return DISPLAY_ILI9341;
    }

    // Default to ST7789 if ILI9341 not detected
    return DISPLAY_ST7789;
}

// --- Public API ---

display_type_t display_init(void) {
    // Initialize SPI
    spi_init(SPI_PORT, TFT_SPI_FREQ);
    gpio_set_function(SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI_MOSI_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI_MISO_PIN, GPIO_FUNC_SPI);

    // CS, DC, RST as GPIO outputs
    gpio_init(TFT_CS_PIN);
    gpio_set_dir(TFT_CS_PIN, GPIO_OUT);
    gpio_put(TFT_CS_PIN, 1); // Deselect

    gpio_init(TFT_DC_PIN);
    gpio_set_dir(TFT_DC_PIN, GPIO_OUT);

    gpio_init(TFT_RST_PIN);
    gpio_set_dir(TFT_RST_PIN, GPIO_OUT);

    // Hardware reset
    gpio_put(TFT_RST_PIN, 1);
    sleep_ms(10);
    gpio_put(TFT_RST_PIN, 0);
    sleep_ms(10);
    gpio_put(TFT_RST_PIN, 1);
    sleep_ms(120);

    // Setup backlight PWM
    gpio_set_function(TFT_BL_PIN, GPIO_FUNC_PWM);
    s_bl_slice = pwm_gpio_to_slice_num(TFT_BL_PIN);
    s_bl_chan = pwm_gpio_to_channel(TFT_BL_PIN);
    pwm_set_wrap(s_bl_slice, 999); // 1000 steps
    pwm_set_chan_level(s_bl_slice, s_bl_chan, 0);
    pwm_set_enabled(s_bl_slice, true);

    // Detect and initialize display
    s_display_type = detect_display();

    if (s_display_type == DISPLAY_ILI9341) {
        init_ili9341();
    } else {
        init_st7789();
    }

    // Set default backlight
    display_set_backlight(BL_DEFAULT_PCT);

    // Clear screen to black
    display_fill(COLOR_BLACK);

    return s_display_type;
}

void display_set_backlight(uint8_t percent) {
    if (percent > 100) percent = 100;
    uint16_t level = (uint16_t)percent * 10; // 0–1000
    pwm_set_chan_level(s_bl_slice, s_bl_chan, level);
}

void display_set_window(int16_t x, int16_t y, int16_t w, int16_t h) {
    uint16_t x1 = x + w - 1;
    uint16_t y1 = y + h - 1;

    // Column address set
    write_cmd(0x2A);
    uint8_t col_data[] = {x >> 8, x & 0xFF, x1 >> 8, x1 & 0xFF};
    write_data(col_data, 4);

    // Row address set
    write_cmd(0x2B);
    uint8_t row_data[] = {y >> 8, y & 0xFF, y1 >> 8, y1 & 0xFF};
    write_data(row_data, 4);

    // Memory write
    write_cmd(0x2C);
}

void display_write_pixels(const uint16_t *data, uint32_t count) {
    dc_data();
    cs_select();
    spi_write_blocking(SPI_PORT, (const uint8_t *)data, count * 2);
    cs_deselect();
}

void display_fill(uint16_t color) {
    display_set_window(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    // Write in row chunks to save stack
    uint16_t row[SCREEN_WIDTH];
    for (int i = 0; i < SCREEN_WIDTH; i++) {
        row[i] = (color >> 8) | (color << 8); // Swap bytes for SPI
    }

    dc_data();
    cs_select();
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        spi_write_blocking(SPI_PORT, (const uint8_t *)row, SCREEN_WIDTH * 2);
    }
    cs_deselect();
}

void display_pixel(int16_t x, int16_t y, uint16_t color) {
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;

    display_set_window(x, y, 1, 1);
    uint16_t c = (color >> 8) | (color << 8); // Byte-swap
    display_write_pixels(&c, 1);
}

void display_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    // Clip
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > SCREEN_WIDTH) w = SCREEN_WIDTH - x;
    if (y + h > SCREEN_HEIGHT) h = SCREEN_HEIGHT - y;
    if (w <= 0 || h <= 0) return;

    display_set_window(x, y, w, h);

    uint16_t swapped = (color >> 8) | (color << 8);
    uint16_t row_buf[SCREEN_WIDTH];
    int fill_w = (w < SCREEN_WIDTH) ? w : SCREEN_WIDTH;
    for (int i = 0; i < fill_w; i++) {
        row_buf[i] = swapped;
    }

    dc_data();
    cs_select();
    for (int row = 0; row < h; row++) {
        spi_write_blocking(SPI_PORT, (const uint8_t *)row_buf, fill_w * 2);
    }
    cs_deselect();
}

void display_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    display_hline(x, y, w, color);
    display_hline(x, y + h - 1, w, color);
    display_vline(x, y, h, color);
    display_vline(x + w - 1, y, h, color);
}

void display_hline(int16_t x, int16_t y, int16_t w, uint16_t color) {
    display_fill_rect(x, y, w, 1, color);
}

void display_vline(int16_t x, int16_t y, int16_t h, uint16_t color) {
    display_fill_rect(x, y, 1, h, color);
}

void display_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    // Bresenham's line algorithm
    int16_t dx = abs(x1 - x0);
    int16_t dy = -abs(y1 - y0);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx + dy;

    while (true) {
        display_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int16_t e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void display_circle(int16_t cx, int16_t cy, int16_t r, uint16_t color) {
    // Midpoint circle algorithm
    int16_t x = r, y = 0, err = 1 - r;

    while (x >= y) {
        display_pixel(cx + x, cy + y, color);
        display_pixel(cx + y, cy + x, color);
        display_pixel(cx - y, cy + x, color);
        display_pixel(cx - x, cy + y, color);
        display_pixel(cx - x, cy - y, color);
        display_pixel(cx - y, cy - x, color);
        display_pixel(cx + y, cy - x, color);
        display_pixel(cx + x, cy - y, color);

        y++;
        if (err < 0) {
            err += 2 * y + 1;
        } else {
            x--;
            err += 2 * (y - x) + 1;
        }
    }
}

void display_fill_circle(int16_t cx, int16_t cy, int16_t r, uint16_t color) {
    int16_t x = r, y = 0, err = 1 - r;

    while (x >= y) {
        display_hline(cx - x, cy + y, 2 * x + 1, color);
        display_hline(cx - y, cy + x, 2 * y + 1, color);
        display_hline(cx - x, cy - y, 2 * x + 1, color);
        display_hline(cx - y, cy - x, 2 * y + 1, color);

        y++;
        if (err < 0) {
            err += 2 * y + 1;
        } else {
            x--;
            err += 2 * (y - x) + 1;
        }
    }
}

void display_char(int16_t x, int16_t y, char c, uint16_t color, uint16_t bg, uint8_t size) {
    if (c < FONT_FIRST_CHAR || c > FONT_LAST_CHAR) return;

    const uint8_t *glyph = font_5x7[c - FONT_FIRST_CHAR];

    for (int col = 0; col < FONT_WIDTH; col++) {
        uint8_t column = glyph[col];
        for (int row = 0; row < FONT_HEIGHT; row++) {
            uint16_t pixel_color = (column & (1 << row)) ? color : bg;
            if (size == 1) {
                display_pixel(x + col, y + row, pixel_color);
            } else {
                display_fill_rect(x + col * size, y + row * size,
                                  size, size, pixel_color);
            }
        }
    }
    // 1-pixel gap between characters
    if (size == 1) {
        display_vline(x + FONT_WIDTH, y, FONT_HEIGHT, bg);
    } else {
        display_fill_rect(x + FONT_WIDTH * size, y, size, FONT_HEIGHT * size, bg);
    }
}

void display_text(int16_t x, int16_t y, const char *str, uint16_t color, uint16_t bg, uint8_t size) {
    int16_t cx = x;
    while (*str) {
        if (*str == '\n') {
            cx = x;
            y += (FONT_HEIGHT + 1) * size;
        } else {
            display_char(cx, y, *str, color, bg, size);
            cx += (FONT_WIDTH + 1) * size;
        }
        str++;
    }
}

int display_text_width(const char *str, uint8_t size) {
    int len = 0;
    while (*str && *str != '\n') {
        len++;
        str++;
    }
    return len * (FONT_WIDTH + 1) * size - size; // Subtract trailing gap
}

void display_spi_acquire(void) {
    cs_deselect(); // Make sure TFT is deselected
}

void display_spi_release(void) {
    // Restore TFT SPI speed after SD card operations
    spi_set_baudrate(SPI_PORT, TFT_SPI_FREQ);
}
