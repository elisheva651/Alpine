#include "map_loader.h"
#include "config.h"
#include "display/display.h"

#include "ff.h"
using namespace fatfs;

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// ============================================================================
// Map Loader — meta.json parsing + row-by-row viewport rendering
// ============================================================================

// Simple JSON value extractor (no full parser needed for our small meta.json)
static bool json_get_string(const char *json, const char *key, char *out, int max_len) {
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char *p = strstr(json, pattern);
    if (!p) return false;
    p = strchr(p + strlen(pattern), '"');
    if (!p) return false;
    p++; // Skip opening quote
    int i = 0;
    while (*p && *p != '"' && i < max_len - 1) {
        out[i++] = *p++;
    }
    out[i] = '\0';
    return true;
}

static bool json_get_int(const char *json, const char *key, int *out) {
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char *p = strstr(json, pattern);
    if (!p) return false;
    p = strchr(p + strlen(pattern), ':');
    if (!p) return false;
    p++;
    while (*p == ' ' || *p == '\t') p++;
    *out = atoi(p);
    return true;
}

static bool json_get_float(const char *json, const char *key, float *out) {
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char *p = strstr(json, pattern);
    if (!p) return false;
    p = strchr(p + strlen(pattern), ':');
    if (!p) return false;
    p++;
    while (*p == ' ' || *p == '\t') p++;
    *out = (float)atof(p);
    return true;
}

bool map_load(const char *map_dir, map_t *map) {
    memset(map, 0, sizeof(map_t));
    snprintf(map->dir_path, sizeof(map->dir_path), "%s", map_dir);

    // Read meta.json
    char meta_path[160];
    snprintf(meta_path, sizeof(meta_path), "%s/meta.json", map_dir);

    FIL file;
    if (f_open(&file, meta_path, FA_READ) != FR_OK) {
        return false;
    }

    char json[512];
    UINT bytes_read;
    FRESULT res = f_read(&file, json, sizeof(json) - 1, &bytes_read);
    f_close(&file);

    if (res != FR_OK || bytes_read == 0) {
        return false;
    }
    json[bytes_read] = '\0';

    // Parse fields
    json_get_string(json, "name", map->meta.name, sizeof(map->meta.name));

    int w, h;
    if (json_get_int(json, "width", &w)) map->meta.width = (uint16_t)w;
    if (json_get_int(json, "height", &h)) map->meta.height = (uint16_t)h;

    // Parse nested coordinates — look for lat/lon after "top_left" and "bottom_right"
    const char *tl = strstr(json, "top_left");
    if (tl) {
        const char *tl_end = strchr(tl, '}');
        if (tl_end) {
            // Extract lat/lon from the top_left block
            char block[128];
            int block_len = tl_end - tl;
            if (block_len < (int)sizeof(block)) {
                memcpy(block, tl, block_len);
                block[block_len] = '\0';
                json_get_float(block, "lat", &map->meta.top_left_lat);
                json_get_float(block, "lon", &map->meta.top_left_lon);
            }
        }
    }

    const char *br = strstr(json, "bottom_right");
    if (br) {
        const char *br_end = strchr(br, '}');
        if (br_end) {
            char block[128];
            int block_len = br_end - br;
            if (block_len < (int)sizeof(block)) {
                memcpy(block, br, block_len);
                block[block_len] = '\0';
                json_get_float(block, "lat", &map->meta.bottom_right_lat);
                json_get_float(block, "lon", &map->meta.bottom_right_lon);
            }
        }
    }

    map->loaded = true;
    return true;
}

bool map_gps_to_pixel(const map_t *map, float lat, float lon,
                      int16_t *px, int16_t *py) {
    if (!map->loaded) return false;

    float dx = map->meta.bottom_right_lon - map->meta.top_left_lon;
    float dy = map->meta.bottom_right_lat - map->meta.top_left_lat;

    if (dx == 0.0f || dy == 0.0f) return false;

    float x = (lon - map->meta.top_left_lon) / dx * map->meta.width;
    float y = (lat - map->meta.top_left_lat) / dy * map->meta.height;

    *px = (int16_t)x;
    *py = (int16_t)y;

    // Check bounds
    return (*px >= 0 && *px < map->meta.width &&
            *py >= 0 && *py < map->meta.height);
}

void map_render_viewport(const map_t *map, int16_t viewport_x, int16_t viewport_y) {
    if (!map->loaded) return;

    // Open the raw RGB565 file
    char img_path[160];
    snprintf(img_path, sizeof(img_path), "%s/map.rgb565", map->dir_path);

    FIL file;
    if (f_open(&file, img_path, FA_READ) != FR_OK) {
        return;
    }

    // Set display window to full screen
    display_set_window(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    // Double-buffer: read one row while sending the previous
    uint16_t buf[2][SCREEN_WIDTH];
    int cur_buf = 0;

    for (int screen_y = 0; screen_y < SCREEN_HEIGHT; screen_y++) {
        int map_y = viewport_y + screen_y;

        if (map_y < 0 || map_y >= map->meta.height) {
            // Out of map bounds — fill with black
            memset(buf[cur_buf], 0, SCREEN_WIDTH * 2);
        } else {
            // Seek to the correct row in the file
            UINT bytes_per_row = map->meta.width * 2;
            FSIZE_t offset = (FSIZE_t)map_y * bytes_per_row + viewport_x * 2;
            f_lseek(&file, offset);

            // Calculate how many pixels to read (handle edge clipping)
            int read_start = 0;
            int read_width = SCREEN_WIDTH;

            if (viewport_x < 0) {
                read_start = -viewport_x;
                read_width -= read_start;
                memset(buf[cur_buf], 0, read_start * 2);
            }
            if (viewport_x + SCREEN_WIDTH > map->meta.width) {
                int overflow = (viewport_x + SCREEN_WIDTH) - map->meta.width;
                read_width -= overflow;
                memset(&buf[cur_buf][SCREEN_WIDTH - overflow], 0, overflow * 2);
            }

            if (read_width > 0) {
                UINT read_bytes;
                f_read(&file, &buf[cur_buf][read_start], read_width * 2, &read_bytes);
            }
        }

        // Write row to display
        display_write_pixels(buf[cur_buf], SCREEN_WIDTH);
        cur_buf ^= 1; // Swap buffer
    }

    f_close(&file);
}

int map_list(char names[][64], int max_maps) {
    DIR dir;
    FILINFO fno;
    int count = 0;

    if (f_opendir(&dir, MAP_DIR) != FR_OK) {
        return 0;
    }

    while (count < max_maps) {
        if (f_readdir(&dir, &fno) != FR_OK || fno.fname[0] == '\0') {
            break;
        }
        if (fno.fattrib & AM_DIR) {
            strncpy(names[count], fno.fname, 63);
            names[count][63] = '\0';
            count++;
        }
    }

    f_closedir(&dir);
    return count;
}
