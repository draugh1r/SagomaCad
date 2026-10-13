#pragma once
#include <cstddef>
#include <span>
#include <string_view>

namespace sagomacad::ui {
struct ToolbarCommand {
    std::string_view id;
    std::string_view label;
    std::string_view icon;
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
bool command_available(std::string_view id, std::span<const std::string_view> registered_ids);
}
