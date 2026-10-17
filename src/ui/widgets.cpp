#include "ui/widgets.hpp"
#include "ui/frame.hpp"
#include <imgui.h>
#include <algorithm>
#include <cfloat>

namespace sagomacad::ui {
namespace {
ImU32 packed(Color c) { return ImGui::ColorConvertFloat4ToU32({c.r,c.g,c.b,c.a}); }
void icon(ImDrawList* dl,unsigned texture,ImVec2 a,float size,ImU32 color) {
    if (!texture)return;
    if(color)dl->AddImage(static_cast<ImTextureID>(texture),a,{a.x+size,a.y+size},{0,0},{1,1},color);
    else dl->AddImage(static_cast<ImTextureID>(texture),a,{a.x+size,a.y+size});
}
}
void ribbon_button(const ToolbarCommand& command,LayoutRect r,bool show_label,const Theme& t,float z,
                   const engine::Engine* engine,const IconTexture& texture,const InvokeCommand& invoke) {
    if (!r.visible()) return;
    auto* dl=ImGui::GetWindowDrawList();
    const auto status=command_availability(command.id,engine);
    const bool available=status.enabled;
    const std::string id(command.id);
    const ImVec2 a{r.x,r.y},size{r.width,r.height};
    ImGui::SetCursorScreenPos(a);
    ImGui::PushID(id.c_str());
    ImGui::BeginDisabled(!available);
    const bool clicked=ImGui::InvisibleButton("command",size);
    ImGui::EndDisabled();
    const bool hovered=ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled);
    if (!available&&hovered) ImGui::SetTooltip("%s",status.reason.c_str());
    if (clicked&&available) invoke(command.id);
    if (hovered) dl->AddRectFilled(a,{a.x+size.x,a.y+size.y},packed(t.panel_alt),t.rounding*z);
    const float logical_icon=show_label?t.board.ribbon_icon:t.layout.menu_icon,icon_size=logical_icon*z;
    icon(dl,texture(command,logical_icon,!available),{a.x+(size.x-icon_size)*0.5f,a.y+t.layout.ribbon_icon_top*z},icon_size,command.design_icon.empty()?packed(available?t.text:t.muted):0);
    if (show_label) {
        const std::string label(command.label);
        const float available_width=std::max(0.0f,size.x-t.spacing*z);
        const float base=t.layout.ribbon_label_size*z;
        const auto unwrapped=ImGui::GetFont()->CalcTextSizeA(base,FLT_MAX,0,label.c_str());
        const float font_size=std::max(t.layout.ribbon_label_min*z,std::min(base,base*available_width/std::max(1.f,unwrapped.x))),wrap=0;
        const auto text_size=ImGui::GetFont()->CalcTextSizeA(font_size,FLT_MAX,wrap,label.c_str());
        dl->AddText(ImGui::GetFont(),font_size,{a.x+(size.x-text_size.x)*0.5f,a.y+(t.layout.ribbon_icon_top+logical_icon+t.layout.ribbon_label_gap)*z},packed(available?t.text:t.muted),label.c_str(),nullptr,wrap);
    }
    ImGui::PopID();
}
void popup_command(const ToolbarCommand& command,float width,const Theme& t,float z,
                   const engine::Engine* engine,const IconTexture& texture,const InvokeCommand& invoke) {
    const auto status=command_availability(command.id,engine);
    const bool available=status.enabled;
    auto* dl=ImGui::GetWindowDrawList();
    ImGui::PushID(command.id.data());
    ImGui::BeginDisabled(!available);
    const bool clicked=ImGui::InvisibleButton("item",{width*z,27*z});
    ImGui::EndDisabled();
    const auto q=ImGui::GetItemRectMin();
    const bool hovered=ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled);
    if (hovered) dl->AddRectFilled(q,{q.x+width*z,q.y+27*z},packed(t.panel_alt));
    icon(dl,texture(command,t.layout.menu_icon,!available),{q.x+4*z,q.y+4*z},t.layout.menu_icon*z,command.design_icon.empty()?packed(available?t.text:t.muted):0);
    dl->AddText({q.x+30*z,q.y+5*z},packed(available?t.text:t.muted),std::string(command.label).c_str());
    if (!available&&hovered) ImGui::SetTooltip("%s",status.reason.c_str());
    if (clicked&&available) { invoke(command.id); ImGui::CloseCurrentPopup(); }
    ImGui::PopID();
}
bool splitter(const char* name,LayoutRect r,bool vertical,float& size,float initial,float minimum,float maximum,float z,const Theme& t) {
    if (!r.visible()) return false;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0,0});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize,{0,0});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,0);
    constexpr auto flags=ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoNav;
    bool changed=false;
    if (frame::panel(name,r,flags)) {
        ImGui::InvisibleButton("splitter",{r.width,r.height});
        const bool hovered=ImGui::IsItemHovered(),active=ImGui::IsItemActive();
        if (hovered||active) {
            ImGui::SetMouseCursor(vertical?ImGuiMouseCursor_ResizeEW:ImGuiMouseCursor_ResizeNS);
            ImGui::GetWindowDrawList()->AddRectFilled({r.x,r.y},{r.x+r.width,r.y+r.height},packed(t.accent));
        }
        if (hovered&&ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) { size=initial; changed=true; }
        else if (active&&ImGui::IsMouseDragging(ImGuiMouseButton_Left,0)) {
            const auto delta=ImGui::GetIO().MouseDelta;
            // Right and bottom panels grow against the pointer's direction.
            const float direction=std::string_view(name)=="browser_separator"?1.0f:-1.0f;
            size=std::clamp(size+direction*(vertical?delta.x:delta.y)/z,minimum,maximum);
            changed=true;
        }
    }
    ImGui::End(); ImGui::PopStyleVar(4);
    return changed;
}
}
