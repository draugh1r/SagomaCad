#pragma once
#include "engine/command.hpp"
namespace sagomacad::engine {
// Read-only geometric decision shared by commands, UI, CLI and MCP.
CommandResult extrusion_defaults(const doc::Document&,const nlohmann::json& params);
}
