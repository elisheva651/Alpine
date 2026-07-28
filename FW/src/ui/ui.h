#ifndef UI_H
#define UI_H

#include <stdint.h>
#include "input/buttons.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// UI State Machine — View management
// ============================================================================

typedef enum {
    VIEW_DASHBOARD = 0,
    VIEW_MAP,
    VIEW_TRACK,
    VIEW_SETTINGS,
    VIEW_COUNT,
} view_t;

// Initialize UI (call after display_init)
void ui_init(void);

// Process button input and update current view
void ui_handle_input(button_event_t event);

// Render current view to display
void ui_render(void);

// Get/set current view
view_t ui_get_view(void);
void ui_set_view(view_t view);

// Individual view render functions (defined in view_*.c files)
void view_dashboard_render(void);
void view_dashboard_input(button_event_t event);

void view_map_render(void);
void view_map_input(button_event_t event);

void view_track_render(void);
void view_track_input(button_event_t event);

void view_settings_render(void);
void view_settings_input(button_event_t event);

#ifdef __cplusplus
}
#endif

#endif // UI_H
