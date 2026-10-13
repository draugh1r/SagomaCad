#pragma once
#include "ui/state.hpp"
#include "engine/command.hpp"
#include "doc/expression.hpp"
#include <string>

namespace sagomacad::ui {
struct ParameterInput {
    std::string name;
    std::string expression;
    std::string error;
};
ParameterInput parse_parameter_input(std::string name, std::string expression);
bool submit_parameter(State& state, engine::Engine& engine);
doc::EvaluatedParameter preview_parameter(const State& state, const engine::Engine& engine);
} // namespace sagomacad::ui
