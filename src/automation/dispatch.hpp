#pragma once
#include "engine/command.hpp"
#include <nlohmann/json.hpp>
#include <string_view>

namespace sagomacad::automation {

// Returns {"result": ...} or {"error": {"message": ...}}; transport adds request id.
nlohmann::json dispatch(engine::Engine& engine, std::string_view method,
                         const nlohmann::json& params);

} // namespace sagomacad::automation
