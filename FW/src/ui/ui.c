#include "ui.h"
#include "config.h"
#include "display/display.h"

// ============================================================================
// UI State Machine — View management and routing
// ============================================================================

static view_t s_current_view = VIEW_DASHBOARD;
static bool s_needs_redraw = true;

void ui_init(void) {
    s_current_view = VIEW_DASHBOARD;
    s_needs_redraw = true;
}

void ui_handle_input(button_event_t event) {
    if (event.type == BTN_EVENT_NONE) return;

    // Select button (short press) cycles through views
    if (event.button == BTN_SELECT && event.type == BTN_EVENT_PRESS) {
        s_current_view = (s_current_view + 1) % VIEW_COUNT;
        s_needs_redraw = true;
        display_fill(COLOR_BLACK); // Clear on view switch
        return;
    }

    // Route directional input to current view
    switch (s_current_view) {
    case VIEW_DASHBOARD: view_dashboard_input(event); break;
    case VIEW_MAP:       view_map_input(event);       break;
    case VIEW_TRACK:     view_track_input(event);     break;
    case VIEW_SETTINGS:  view_settings_input(event);  break;
    default: break;
    }

    s_needs_redraw = true;
}

void ui_render(void) {
    if (!s_needs_redraw) return;

    switch (s_current_view) {
    case VIEW_DASHBOARD: view_dashboard_render(); break;
    case VIEW_MAP:       view_map_render();       break;
    case VIEW_TRACK:     view_track_render();     break;
    case VIEW_SETTINGS:  view_settings_render();  break;
    default: break;
    }

    s_needs_redraw = false;
}

view_t ui_get_view(void) {
    return s_current_view;
}

void ui_set_view(view_t view) {
    if (view < VIEW_COUNT) {
        s_current_view = view;
        s_needs_redraw = true;
        display_fill(COLOR_BLACK);
    }
}
