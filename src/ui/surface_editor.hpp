#pragma once
#include "ui/state.hpp"
#include "engine/command.hpp"
namespace sagomacad::ui {
void begin_surface(State&,engine::Engine&,const std::string& command,const std::string& editing={});
nlohmann::json surface_params(const State&);
bool submit_surface(State&,engine::Engine&);
}
