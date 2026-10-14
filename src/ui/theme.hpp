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
    const char* parameters;
};
inline constexpr SectionLabels labels {
    "BROWSER", "DOCUMENTO", "CORPI", "PROPRIETÀ", "SELEZIONE",
    "TIMELINE", "CRONOLOGIA DELLE FEATURE", "PARAMETRI"
};
struct ParameterTableLabels {
    const char* name;
    const char* unit;
    const char* expression;
    const char* value;
    const char* comment;
};
inline constexpr ParameterTableLabels parameter_table_labels {
    "Nome", "Unità", "Espressione", "Valore", "Commento"
};
struct ViewCubeTokens {
    float widget_size=156.0f, cube_size=76.0f, center_x=78.0f, center_y=84.0f;
    float bevel=0.18f, control_size=20.0f, label_size=13.0f, line_width=1.0f;
    float projection_scale=0.42f;
    float axis_length=28.0f, axis_label_gap=5.0f, tooltip_delay=0.6f;
    std::array<float,2> axis_origin{{20,138}};
    std::array<std::array<float,2>,7> control_positions{{{16,16},{78,27},{78,141},{21,84},{135,84},{138,36},{112,16}}};
    Color axis_z{0.20f,0.47f,0.95f}, hover{0.25f,0.65f,1.0f,0.45f};
    const char* top="TOP";
    const char* bottom="BOTTOM";
    const char* front="FRONT";
    const char* back="BACK";
    const char* right="RIGHT";
    const char* left="LEFT";
};
struct Theme {
    Color background, panel, panel_alt, viewport, text, muted, border, accent, axis_x, axis_y, grid_major, grid_minor;
    Color table_header, table_header_text, error, error_cell;
    Color solid, solid_edge, cube_face, cube_edge;
    float rounding, spacing, padding, sidebar_width, inspector_width, timeline_height, toolbar_height;
    float grid_step, grid_slope, viewcube_size, nav_width, nav_height, ribbon_group_width, timeline_glyph_height;
    float palette_width, palette_height, parameter_width, parameter_height, file_dialog_width;
    ViewCubeTokens cube;
};
inline constexpr Theme dark {
    {0.055f,0.065f,0.080f}, {0.090f,0.105f,0.125f}, {0.115f,0.135f,0.160f},
    {0.075f,0.095f,0.120f}, {0.93f,0.95f,0.97f}, {0.56f,0.61f,0.66f},
    {0.19f,0.22f,0.26f}, {0.25f,0.58f,0.91f}, {0.87f,0.30f,0.28f},
    {0.34f,0.75f,0.47f}, {0.18f,0.23f,0.29f}, {0.12f,0.16f,0.20f},
    {0.16f,0.19f,0.22f}, {0.94f,0.96f,0.98f}, {1.0f,0.43f,0.39f}, {0.28f,0.12f,0.13f},
    {0.42f,0.64f,0.80f}, {0.04f,0.08f,0.13f}, {0.19f,0.25f,0.31f}, {0.48f,0.57f,0.66f},
    3.0f, 7.0f, 10.0f, 244.0f, 254.0f, 62.0f, 164.0f,
    25.0f, 0.52f, 68.0f, 224.0f, 30.0f, 180.0f, 124.0f,
    480.0f, 320.0f, 800.0f, 420.0f, 480.0f
};
inline constexpr Theme light {
    {0.95f,0.96f,0.97f}, {0.985f,0.988f,0.992f}, {0.94f,0.95f,0.96f},
    {0.995f,0.997f,1.0f}, {0.17f,0.20f,0.23f}, {0.40f,0.44f,0.48f},
    {0.78f,0.80f,0.82f}, {0.08f,0.40f,0.73f}, {0.82f,0.33f,0.32f},
    {0.31f,0.68f,0.40f}, {0.84f,0.86f,0.88f}, {0.91f,0.92f,0.93f},
    {0.88f,0.90f,0.92f}, {0.17f,0.20f,0.23f}, {0.73f,0.16f,0.14f}, {0.98f,0.87f,0.86f},
    {0.64f,0.78f,0.87f}, {0.19f,0.27f,0.35f}, {0.88f,0.92f,0.96f}, {0.39f,0.48f,0.58f},
    3.0f, 7.0f, 10.0f, 244.0f, 254.0f, 62.0f, 164.0f,
    25.0f, 0.52f, 68.0f, 224.0f, 30.0f, 180.0f, 124.0f,
    480.0f, 320.0f, 800.0f, 420.0f, 480.0f
};
inline constexpr const Theme& theme(bool is_light) { return is_light ? light : dark; }
}
