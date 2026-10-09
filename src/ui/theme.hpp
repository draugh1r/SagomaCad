#pragma once
#include <array>

namespace sagomacad::ui {
struct Color { float r, g, b, a = 1.0f; };
struct SectionLabels {
    const char* browser;
    const char* document;
    const char* bodies;
    const char* properties;
    const char* selection;
    const char* timeline;
    const char* feature_history;
};
inline constexpr SectionLabels labels {
    "BROWSER", "DOCUMENTO", "CORPI", "PROPRIETÀ", "SELEZIONE",
    "TIMELINE", "CRONOLOGIA DELLE FEATURE"
};
struct Theme {
    Color background, panel, panel_alt, viewport, text, muted, border, accent, axis_x, axis_y, grid_major, grid_minor;
    float rounding, spacing, padding, sidebar_width, inspector_width, timeline_height, toolbar_height;
};
inline constexpr Theme dark {
    {0.055f,0.065f,0.080f}, {0.090f,0.105f,0.125f}, {0.115f,0.135f,0.160f},
    {0.075f,0.095f,0.120f}, {0.93f,0.95f,0.97f}, {0.56f,0.61f,0.66f},
    {0.19f,0.22f,0.26f}, {0.25f,0.58f,0.91f}, {0.87f,0.30f,0.28f},
    {0.34f,0.75f,0.47f}, {0.18f,0.23f,0.29f}, {0.12f,0.16f,0.20f},
    5.0f, 8.0f, 12.0f, 220.0f, 254.0f, 122.0f, 66.0f
};
inline constexpr Theme light {
    {0.89f,0.91f,0.93f}, {0.97f,0.98f,0.99f}, {0.92f,0.94f,0.96f},
    {0.87f,0.91f,0.95f}, {0.12f,0.16f,0.20f}, {0.39f,0.45f,0.50f},
    {0.77f,0.81f,0.85f}, {0.12f,0.42f,0.76f}, {0.75f,0.19f,0.18f},
    {0.15f,0.55f,0.28f}, {0.72f,0.78f,0.84f}, {0.80f,0.85f,0.89f},
    5.0f, 8.0f, 12.0f, 220.0f, 254.0f, 122.0f, 66.0f
};
inline constexpr const Theme& theme(bool is_light) { return is_light ? light : dark; }
}
