#pragma once
#include <array>
#include <string_view>

namespace sagomacad::ui {
struct Color { float r, g, b, a = 1.0f; };
constexpr Color rgb(unsigned value,float alpha=1) {
    return {float((value>>16)&255)/255,float((value>>8)&255)/255,float(value&255)/255,alpha};
}
// Name, light, dark, opacity. One list drives both themes, the board and validation.
#define SAGOMACAD_VISUAL_COLORS(X) \
    X(app_bar,0xffffff,0x191f1b,1) \
    X(toolbar,0xffffff,0x202821,1) \
    X(panel_header,0xffffff,0x2b352c,1) \
    X(hover,0xe6eed9,0x35442d,1) \
    X(accent_soft,0xe1edce,0x34472a,1) \
    X(text_on_accent,0x243019,0x243019,1) \
    X(sel_face,0x91b65d,0xb5d18b,.42f) \
    X(sel_edge,0x54752d,0xb5d18b,1) \
    X(sel_edge_rim,0xc0d49e,0x617e43,1) \
    X(hover_edge,0x2a3622,0xe7efdc,1) \
    X(hover_edge_halo,0xd4dfc7,0x53624a,.8f) \
    X(hover_face,0xffffff,0xffffff,.16f) \
    X(sel_profile,0x91b65d,0xb5d18b,.34f) \
    X(sel_profile_edge,0x54752d,0xb5d18b,1) \
    X(hover_profile,0xb1cc8e,0x94ba68,.22f) \
    X(hover_profile_edge,0x6d9145,0xc4df9b,1) \
    X(sel_vertex,0x54752d,0xc4df9b,1) \
    X(origin_plane,0xe7ba7c,0xd8a569,.20f) \
    X(origin_plane_edge,0x9f702c,0xe5b77b,.75f) \
    X(origin_plane_hover,0xc2d6a4,0xa4c478,.45f) \
    X(origin_point,0x666666,0xcbd5bf,1) \
    X(construction_plane,0xe7ba7c,0xd8a569,.16f) \
    X(preview_add,0x5b9bc9,0x79b4dd,.35f) \
    X(preview_new,0x9fc472,0xb5d18b,.35f) \
    X(preview_cut,0xd86550,0xef8873,.35f) \
    X(preview_sketch,0x679249,0xa5c980,.45f) \
    X(viewport_top,0xffffff,0x202820,1) \
    X(viewport_bottom,0xffffff,0x151c17,1) \
    X(body_selected,0xa6c77d,0xb2cc8e,1) \
    X(dimension,0x333333,0xe3ecd9,1) \
    X(dimension_driven,0x666666,0xa4b297,1) \
    X(sketch_projected,0x8852ad,0xbf8de1,1) \
    X(sketch_lost,0xb24759,0xf08b9a,1) \
    X(sketch_construction,0xac702b,0xe4af67,1) \
    X(sketch_centerline,0xa06b36,0xd8a36a,1) \
    X(constraint,0xb34d42,0xe38c7e,1) \
    X(glyph_bg,0xffffff,0x242d24,.94f) \
    X(glyph_edge,0xd9b4a9,0x80574a,1) \
    X(box_window,0x4f86ad,0x82bbdd,.30f) \
    X(box_crossing,0x70994d,0xa5cb79,.30f) \
    X(timeline_item,0xffffff,0x303b2e,1) \
    X(timeline_error,0xf9dfd8,0x5c332d,1) \
    X(timeline_marker,0x4d603c,0xb8d48f,1) \
    X(dialog_bg,0xffffff,0x242e25,1) \
    X(dialog_border,0xd8d8d8,0x4e5d46,1) \
    X(heading_bg,0xffffff,0x35412f,1) \
    X(subheading_bg,0xffffff,0x2c3728,1) \
    X(field,0xffffff,0x1c251d,1) \
    X(overlay,0xffffff,0x242e25,.94f) \
    X(warning,0xa06a1d,0xe5b15c,1)
struct VisualTokens {
#define SAGOMA_FIELD(name,l,d,a) Color name{};
    SAGOMACAD_VISUAL_COLORS(SAGOMA_FIELD)
#undef SAGOMA_FIELD
};
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
    float hit_band=0.26f, highlight_band=0.12f, hit_padding=4.0f;
    float transition_duration=0.85f;
    float control_size=20.0f, label_size=13.0f, label_inset=0.18f, line_width=1.0f;
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
    struct LayoutTokens {
        float topbar_height=66, divider=2, panel_border=1;
        float browser_min=160, browser_max=480, properties_min=160, properties_max=480;
        float timeline_min=62, timeline_max=240, viewport_min_width=320, viewport_min_height=200;
        float collapsed_width=24, ribbon_top=4, ribbon_bottom=12, ribbon_right=14;
        float group_button_height=43, group_spacer=15, group_menu_height=22;
        float group_left=2, group_right=6, menu_left=4, menu_right=6;
        float label_top=4, label_right=10, label_font_size=15;
        float menu_icon_size=13, menu_icon_right=19, collapse_button=20;
        float ribbon_label_size=12.5f,ribbon_label_min=10;
        float ribbon_icon_top=2,ribbon_label_gap=2,menu_icon=16,menu_row=27;
    } layout;
    struct SketchTokens {
        Color free{0.12f,0.40f,0.90f}, constrained{0.05f,0.05f,0.06f},profile{0.25f,0.70f,0.90f,0.14f},plane{0.65f,0.64f,0.60f,0.20f},plane_hover{0.98f,0.66f,0.12f,0.55f},finish{0.12f,0.65f,0.29f};
        float palette_width=244,palette_y=180,palette_height=340,field_width=132,field_height=26,field_gap=8,point_radius=3,hit_radius=10,line_width=1.6f,active_line_width=2.6f,constraint_size=12,dimension_offset=32,dimension_arrow=4,plane_extent=18,grid_pixel_step=28;
        const char* palette="TAVOLOZZA DELLO SCHIZZO";
        const char* finish_label="Fine schizzo";
        const char* select_plane="Seleziona un piano";
    } sketch;
    struct TimelineTokens {
        float icon=20,item=30,gap=4,control=22,marker_width=3,marker_hit=9,marker_height=32,row_top=3,play_interval=.6f;
    } timeline;
    struct ModelTokens {
        float width=292,height=326,revolve_height=270,top=184,row=24,label_width=94,footer=38,field_width=126,field_height=26;
        float handle_hit=12,arrow_width=8,arrow_length=12,axis_extent=60,arc_radius=5,value_gap=12;
        Color selected{.12f,.45f,1.f,.40f},preview{.12f,.45f,1.f,.26f},cut{.95f,.18f,.16f,.32f},axis{1.f,.58f,.08f,1.f};
    } model;
    struct SelectionTokens {
        Color face_hover{1,1,1,.15f},edge_hover{.96f,.97f,.99f,1},reference{.65f,.25f,.85f,1};
        float edge_width=3,face_width=1.4f,edge_hit=4,hold_seconds=.45f,label_size=9,filter_width=220,dialog_width=360,dialog_height=340,hole_height=430;
    } selection;
    VisualTokens visual;
    struct FrameTokens {
        float dialog_min=244,dialog_max=380,window_max=500,window_wide_max=800;
        float heading_height=24,min_height=80,margin=12;
    } frame;
    struct RenderTokens {float edge_px=1.2f,selected_px=3,hover_px=3,halo_px=5;} render;
    struct BoardTokens {
        float margin=24,heading=25,row=32,icon_row=58,swatch=20;
        float columns=4,example_height=200;
        std::array<float,3> icon_sizes{{16,24,32}};
        float ribbon_icon=26,browser_icon=16,disabled_icon=24;
    } board;
};
inline constexpr Theme dark_base {
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
inline constexpr Theme light_base {
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
constexpr Theme make_theme(Theme t,bool light) {
#define SAGOMA_VALUE(name,l,d,a) t.visual.name=rgb(light?l:d,a);
    SAGOMACAD_VISUAL_COLORS(SAGOMA_VALUE)
#undef SAGOMA_VALUE
    t.accent=rgb(light?0x93c572:0xb1ce8a);
    t.background=rgb(light?0xffffff:0x192119);
    t.panel=t.visual.dialog_bg;t.panel_alt=t.visual.subheading_bg;
    t.viewport=t.visual.viewport_top;t.border=t.visual.dialog_border;
    t.text=rgb(light?0x292929:0xe6eddd);t.muted=rgb(light?0x666666:0xa7b59a);
    t.table_header=t.visual.heading_bg;t.table_header_text=t.text;
    t.sketch.constrained=t.text;t.sketch.finish=t.accent;
    t.sketch.free=rgb(light?0x3478bd:0x88b8ee);
    t.sketch.plane=t.visual.origin_plane;t.sketch.plane_hover=t.visual.origin_plane_hover;
    t.model.selected=t.visual.sel_face;t.model.preview=t.visual.preview_new;t.model.cut=t.visual.preview_cut;
    t.selection.face_hover=t.visual.hover_face;t.selection.edge_hover=t.visual.hover_edge;
    t.selection.reference=t.visual.sketch_projected;
    t.toolbar_height=156;t.layout.group_spacer=8;
    return t;
}
inline constexpr Theme light=make_theme(light_base,true),dark=make_theme(dark_base,false);
inline constexpr const Theme& theme(bool is_light) { return is_light ? light : dark; }
struct NamedColor {std::string_view name;Color value;};
inline auto theme_colors(const Theme& t) {
    return std::array{
        NamedColor{"background",t.background},NamedColor{"panel",t.panel},NamedColor{"panel_alt",t.panel_alt},
        NamedColor{"viewport",t.viewport},NamedColor{"text",t.text},NamedColor{"muted",t.muted},NamedColor{"border",t.border},NamedColor{"accent",t.accent},
        NamedColor{"axis_x",t.axis_x},NamedColor{"axis_y",t.axis_y},NamedColor{"axis_z",t.cube.axis_z},
        NamedColor{"grid_major",t.grid_major},NamedColor{"grid_minor",t.grid_minor},
        NamedColor{"table_header",t.table_header},NamedColor{"table_header_text",t.table_header_text},
        NamedColor{"error",t.error},NamedColor{"error_cell",t.error_cell},NamedColor{"solid",t.solid},NamedColor{"solid_edge",t.solid_edge},
        NamedColor{"cube_face",t.cube_face},NamedColor{"cube_edge",t.cube_edge},NamedColor{"cube_hover",t.cube.hover},
        NamedColor{"sketch_free",t.sketch.free},NamedColor{"sketch_constrained",t.sketch.constrained},NamedColor{"sketch_profile",t.sketch.profile},
        NamedColor{"sketch_finish",t.sketch.finish},NamedColor{"model_axis",t.model.axis},
#define SAGOMA_NAMED(name,l,d,a) NamedColor{#name,t.visual.name},
        SAGOMACAD_VISUAL_COLORS(SAGOMA_NAMED)
#undef SAGOMA_NAMED
    };
}
}
