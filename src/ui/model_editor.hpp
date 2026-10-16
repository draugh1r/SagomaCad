#pragma once
#include "ui/state.hpp"
#include "engine/command.hpp"
namespace sagomacad::ui {
void begin_model(State&,engine::Engine&,bool revolve,const std::string& editing={});
nlohmann::json model_params(const State&);
bool submit_model(State&,engine::Engine&);
void cancel_model(State&);
nlohmann::json model_state_json(const State&);
}
