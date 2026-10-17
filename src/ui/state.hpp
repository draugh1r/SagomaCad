#pragma once
#include <array>
#include <string>
#include <map>
#include "ui/camera.hpp"
#include "ui/view_cube.hpp"
#include "ui/theme.hpp"
#include <nlohmann/json_fwd.hpp>
#include "engine/tools/sketch_tool.hpp"
#include "engine/picking.hpp"

namespace sagomacad::ui {
struct State {
    struct SurfaceEditor {
        bool open=false,through=false,dragging=false;
        int drag_dimension=-1;
        std::string command,feature_id,error,preview_error;
        nlohmann::json face=nullptr,position=nullptr,edges=nlohmann::json::array(),references=nlohmann::json::array();
        std::array<char,4096> value{},diameter{},depth{};
        std::array<std::array<char,4096>,2> offsets{};
        float drag_x=0,drag_y=0,drag_px=0,drag_py=0,handle_x=0,handle_y=0,value_x=0,value_y=0;
        double drag_amount=0;
        std::map<std::string,std::array<float,4>> fields;
    } surface;
    engine::PickFilter pick_filter;
    std::optional<engine::PickRef> hovered_entity;
    bool selection_filters_open=false,show_topology_labels=false;
    nlohmann::json topology_labels=nlohmann::json::array();
    struct ModelEditor {
        nlohmann::json face=nullptr;
        bool open=false,revolve=false,pick_axis=false,dragging=false,operation_menu=false;
        std::string feature_id,sketch,axis,error,preview_error;
        std::vector<std::string> profiles,targets;
        int direction=0,extent=0,operation=3,type=0;
        std::array<char,4096> distance{},second_distance{},angle{};
        float drag_x=0,drag_y=0,drag_px=0,drag_py=0,handle_x=0,handle_y=0,value_x=0,value_y=0;
        double drag_amount=0;
        std::map<std::string,std::array<float,4>> fields;
    } model;
    bool model_profile_under_pointer=false;
    std::string selected_profile_sketch;
    std::vector<std::string> selected_profiles;
    struct SketchBrowserItem {std::array<float,4> row{},eye{};bool visible=false;};
    std::map<std::string,SketchBrowserItem> sketch_browser;
    Camera camera;
    CameraTransition view_transition;
    CubeInteraction cube_interaction;
    int navigation_mode = -1; // -1: pointer, 0: pan, 1: orbit, 2: zoom
    std::string theme = "light";
    bool browser = true;
    bool properties = false;
    bool timeline = true;
    bool timeline_dragging=false,timeline_playing=false;
    float timeline_elapsed=0,timeline_marker_x=0,timeline_marker_y=0;
    struct TimelineItem {std::string id;float x=0,y=0,width=0,height=0;};
    std::vector<TimelineItem> timeline_items;
    bool browser_collapsed = false, properties_collapsed = false;
    float browser_width = light.sidebar_width-light.layout.panel_border;
    float properties_width = light.inspector_width-light.layout.panel_border;
    float timeline_height = light.timeline_height;
    bool sketch_mode = false;
    bool sketch_plane_picker=false;
    std::string sketch_view_id,sketch_drag_point,sketch_dimension_id,sketch_dimension_kind="length";
    std::vector<std::string> sketch_selection;
    engine::SketchTool sketch_tool;
    bool sketch_grid=true,sketch_snap=true,sketch_profiles=true,sketch_points=true,sketch_dimensions=true,sketch_constraints=true,sketch_construction=true;
    bool sketch_dimension_open=false,sketch_constraint_open=false;
    int sketch_constraint_kind=0;
    std::array<char,4096> sketch_dimension_expression{};
    std::string sketch_error;
    bool sketch_field_focus=false;
    std::array<std::array<char,4096>,2> sketch_fields{};
    int selected_tab = 0;
    bool previous_sketch_mode = false;
    bool palette_open = false;
    bool palette_focus = false;
    bool parameters_open = false;
    bool file_dialog_open = false;
    bool primitive_editor_open = false;
    bool primitive_create_open = false;
    int primitive_kind = 0;
    std::string primitive_feature_id;
    std::string primitive_error;
    std::string primitive_preview_error;
    std::array<std::array<char,4096>,3> primitive_dimensions{};
    std::string file_command;
    std::string status_message;
    std::string parameter_error;
    std::string editing_parameter_name;
    bool parameter_editing = false;
    std::array<char, 128> palette_query{};
    std::array<char, 128> parameter_name{};
    std::array<char, 4096> parameter_expression{};
    std::array<char, 16> parameter_unit{};
    std::array<char, 513> parameter_comment{};
    std::array<char, 1024> file_path{};
    int width = 1440;
    int height = 900;
};
nlohmann::json layout_state_json(const State&);
void restore_layout_state(State&, const nlohmann::json&);
}
