#pragma once
#include <array>
#include <string>

namespace sagomacad::ui {
struct State {
    std::string theme = "light";
    bool browser = true;
    bool properties = false;
    bool timeline = true;
    bool sketch_mode = false;
    bool palette_open = false;
    bool palette_focus = false;
    bool parameters_open = false;
    bool file_dialog_open = false;
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
