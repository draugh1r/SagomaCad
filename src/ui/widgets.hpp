#pragma once
#include "ui/layout.hpp"
#include "ui/toolbar_catalog.hpp"
#include <functional>

namespace sagomacad::ui {
using IconTexture=std::function<unsigned int(const ToolbarCommand&,float,bool)>;
using InvokeCommand=std::function<void(std::string_view)>;
void ribbon_button(const ToolbarCommand&, LayoutRect, bool show_label, const Theme&, float scale,
                   const engine::Engine*, const IconTexture&, const InvokeCommand&);
void popup_command(const ToolbarCommand&, float width, const Theme&, float scale,
                   const engine::Engine*, const IconTexture&, const InvokeCommand&);
// Returns true while dragging or on reset. Keeps the full requested size in State;
// the layout independently fits it into smaller windows.
bool splitter(const char* id,LayoutRect,bool vertical,float& size,float initial,float minimum,float maximum,float scale,const Theme&);
}
