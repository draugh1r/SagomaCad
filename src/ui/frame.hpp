#pragma once
#include "ui/layout.hpp"
#include <imgui.h>
#include <span>
#include <string>
#include <string_view>
namespace sagomacad::ui::frame {
struct Record {std::string name,kind;LayoutRect rect;float minimum=0,maximum=0;};
void reset_records();
std::span<const Record> records();
// Shell rectangles and docked areas come from Clay. Projection overlays retain their anchor.
bool panel(const char* title,LayoutRect rect,ImGuiWindowFlags flags=0);
bool dialog(const char* title,bool* open,LayoutRect rect,const Theme&,float scale,ImGuiWindowFlags flags=0);
bool window(const char* title,bool* open,const Theme&,float scale,ImVec2 preferred,bool wide=false,ImGuiWindowFlags flags=0);
bool overlay(const char* title,LayoutRect rect,ImGuiWindowFlags flags=0);
bool notice(const char* title,bool* open,LayoutRect viewport,const Theme&,float scale);
void heading(std::string_view text,const Theme&,float scale,bool secondary=false,float reserved=0);
}
