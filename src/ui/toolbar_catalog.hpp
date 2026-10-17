#pragma once
#include "engine/command.hpp"
#include "ui/icons.hpp"
#include <cstddef>
#include <span>
#include <string>
#include <string_view>

namespace sagomacad::ui {
struct ToolbarCommand {
    std::string_view id;
    std::string_view label;
    std::string_view icon;
    std::string_view design_icon; // Empty explicitly means Lucide fallback.
    IconAccent accent=IconAccent::Solid;
};
struct ToolbarGroup {
    std::string_view label;
    std::span<const ToolbarCommand> commands;
    size_t primary_count;
};
struct ToolbarTab {
    std::string_view label;
    bool contextual;
    std::span<const ToolbarGroup> groups;
};

std::span<const ToolbarTab> toolbar_catalog();
std::span<const ToolbarCommand> file_menu_commands();
std::span<const ToolbarCommand> edit_toolbar_commands();
const ToolbarCommand* toolbar_command(std::string_view id);
struct CommandAvailability {
    bool registered = false;
    bool enabled = false;
    std::string reason = "Non ancora disponibile";
};
CommandAvailability command_availability(std::string_view id, const engine::Engine* engine);
}
