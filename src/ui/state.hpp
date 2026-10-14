#pragma once
#include <array>
#include <string>
#include "ui/camera.hpp"

namespace sagomacad::ui {
struct State {
    Camera camera;
    int navigation_mode = -1; // -1: pointer, 0: pan, 1: orbit, 2: zoom
    std::string theme = "light";
    bool browser = true;
    bool properties = false;
    bool timeline = true;
    bool sketch_mode = false;
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
}
