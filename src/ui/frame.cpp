#include "ui/frame.hpp"
#include <algorithm>
#include <vector>
namespace sagomacad::ui::frame {
namespace {
thread_local std::vector<Record> opened;
bool begin(const char* title,bool* open,ImGuiWindowFlags flags,const char* kind,float minimum=0,float maximum=0) {
    const bool visible=ImGui::Begin(title,open,flags);
    const auto p=ImGui::GetWindowPos(),s=ImGui::GetWindowSize();
    opened.push_back({title,kind,{p.x,p.y,s.x,s.y},minimum,maximum});return visible;
}
ImU32 packed(Color c){return ImGui::ColorConvertFloat4ToU32({c.r,c.g,c.b,c.a});}
}
void reset_records(){opened.clear();}
std::span<const Record> records(){return opened;}
bool panel(const char* title,LayoutRect r,ImGuiWindowFlags flags) {
    ImGui::SetNextWindowPos({r.x,r.y});ImGui::SetNextWindowSize({r.width,r.height});
    return begin(title,nullptr,flags,"panel");
}
bool overlay(const char* title,LayoutRect r,ImGuiWindowFlags flags) {
    ImGui::SetNextWindowPos({r.x,r.y});ImGui::SetNextWindowSize({r.width,r.height});
    return begin(title,nullptr,flags,"overlay");
}
bool dialog(const char* title,bool* open,LayoutRect r,const Theme& t,float z,ImGuiWindowFlags flags) {
    const auto display=ImGui::GetIO().DisplaySize;
    const float available=std::max(1.f,display.x-2*t.frame.margin*z);
    const float maximum=std::min(t.frame.dialog_max*z,available),minimum=std::min(t.frame.dialog_min*z,maximum);
    const float width=std::clamp(r.width,minimum,maximum);
    const float x=std::clamp(r.x+r.width-width,0.f,std::max(0.f,display.x-width));
    const float height=std::min(r.height,std::max(1.f,display.y-r.y-t.frame.margin*z));
    ImGui::SetNextWindowPos({x,r.y});ImGui::SetNextWindowSize({width,height});
    ImGui::SetNextWindowSizeConstraints({minimum,1},{maximum,std::max(1.f,display.y-r.y)});
    constexpr auto fixed=ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoDocking;
    const bool visible=begin(title,open,flags|fixed,"dialog",minimum,maximum);
    const std::string_view label(title);const auto end=label.find("##");
    if(visible&&end!=0)heading(label.substr(0,end),t,z);
    return visible;
}
bool window(const char* title,bool* open,const Theme& t,float z,ImVec2 preferred,bool wide,ImGuiWindowFlags flags) {
    const auto display=ImGui::GetIO().DisplaySize;
    const float maximum=std::min((wide?t.frame.window_wide_max:t.frame.window_max)*z,std::max(1.f,display.x-2*t.frame.margin*z));
    const float minimum=std::min(t.frame.dialog_min*z,maximum);
    const float max_height=std::max(1.f,display.y-2*t.frame.margin*z);
    const float width=std::clamp(preferred.x,minimum,maximum);
    const float height=preferred.y>0?std::min(preferred.y,max_height):0;
    ImGui::SetNextWindowPos({display.x*.5f,display.y*.5f},ImGuiCond_Appearing,{.5f,.5f});
    ImGui::SetNextWindowSize({width,height},ImGuiCond_Appearing);
    ImGui::SetNextWindowSizeConstraints({minimum,std::min(t.frame.min_height*z,max_height)},{maximum,max_height});
    return begin(title,open,flags|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoSavedSettings,"window",minimum,maximum);
}
bool notice(const char* title,bool* open,LayoutRect r,const Theme& t,float z) {
    const float width=std::max(1.f,std::min(t.frame.window_max*z,r.width-2*t.padding*z));
    ImGui::SetNextWindowPos({r.x+t.padding*z,r.y+r.height-t.padding*z},ImGuiCond_Always,{0,1});
    ImGui::SetNextWindowSize({width,0});
    ImGui::SetNextWindowSizeConstraints({std::min(t.frame.dialog_min*z,width),1},{width,std::max(1.f,r.height-2*t.padding*z)});
    return begin(title,open,ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings,"notice",std::min(t.frame.dialog_min*z,width),width);
}
void heading(std::string_view text,const Theme& t,float z,bool secondary,float reserved) {
    auto p=ImGui::GetCursorScreenPos();const float width=std::max(0.f,ImGui::GetContentRegionAvail().x-reserved),height=t.frame.heading_height*z;
    auto* dl=ImGui::GetWindowDrawList();dl->AddRectFilled(p,{p.x+width,p.y+height},packed(secondary?t.visual.subheading_bg:t.visual.heading_bg),t.rounding*z);
    dl->AddText({p.x+t.spacing*z,p.y+(height-ImGui::GetFontSize())*.5f},packed(t.text),text.data(),text.data()+text.size());
    ImGui::Dummy({width,height});
}
}
