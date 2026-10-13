#pragma once
#include <nlohmann/json.hpp>
#include <functional>
#include <string_view>
#include "engine/command.hpp"

using McpInvoke=std::function<nlohmann::json(std::string_view,const nlohmann::json&)>;
nlohmann::json mcp_visual_tool(std::string_view name,const nlohmann::json& arguments,
                               const McpInvoke& commands,bool bridge,sagomacad::engine::Engine& local_engine);
