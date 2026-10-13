#pragma once
#include <nlohmann/json.hpp>
#include <string>

namespace sagomacad::automation {

bool control_call(int port, const std::string& token, const std::string& method,
                  const nlohmann::json& params, nlohmann::json& response, std::string& error);

} // namespace sagomacad::automation
