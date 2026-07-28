#ifndef MAP_LOADER_H
#define MAP_LOADER_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Map Loader — meta.json parsing + viewport rendering
// ============================================================================

typedef struct {
    char name[64];
    uint16_t width;         // Map image width in pixels
    uint16_t height;        // Map image height in pixels
    float top_left_lat;
    float top_left_lon;
    float bottom_right_lat;
    float bottom_right_lon;
} map_meta_t;

typedef struct {
    map_meta_t meta;
    char dir_path[128];     // Path to map directory on SD
    bool loaded;
} map_t;

// Load map metadata from /maps/<name>/meta.json
bool map_load(const char *map_dir, map_t *map);

// Convert GPS coords to pixel position on map
bool map_gps_to_pixel(const map_t *map, float lat, float lon,
                      int16_t *px, int16_t *py);

// Render a viewport from the map to the display
void map_render_viewport(const map_t *map, int16_t viewport_x, int16_t viewport_y);

// List available maps on SD card
int map_list(char names[][64], int max_maps);

#ifdef __cplusplus
}
#endif

#endif // MAP_LOADER_H
