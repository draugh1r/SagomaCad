#pragma once
#include "engine/command.hpp"
#include <array>
namespace sagomacad::engine {
struct SketchTool {
    std::string kind="select",error;
    bool started=false;
    double x=0,y=0,cursor_x=0,cursor_y=0;
    int active_field=0,arc_stage=0;
    double arc_start=0;
    double arc_radius=0;
    std::array<std::string,2> fields;
    std::array<bool,2> locked{false,false};
    void begin(std::string tool);
    void move(double u,double v);
    void click(double u,double v);
    void text(std::string_view text);
    void tab();
    void escape();
    std::string hint() const;
    nlohmann::json parameters() const;
    std::array<std::string,2> live_values() const;
    nlohmann::json preview_command() const;
    CommandResult confirm(Engine& engine);
};
}
