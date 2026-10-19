#include "ui/handles.hpp"
#include <imgui.h>
#include <cmath>
namespace sagomacad::ui {
namespace {Handles* current=nullptr;bool name_ok(std::string_view name){return !name.empty()&&name.size()<=512;}}
void Handles::clear(){entries_.clear();}
void Handles::point(std::string_view name,double x,double y){
    if(!name_ok(name)||entries_.size()>=16384||!std::isfinite(x)||!std::isfinite(y)||std::abs(x)>1e7||std::abs(y)>1e7)return;
    entries_[std::string(name)]={"point",{float(x),float(y),0,0}};
}
void Handles::rect(std::string_view name,LayoutRect r,bool enabled){
    if(!name_ok(name)||entries_.size()>=16384||!r.visible()||!std::isfinite(r.x)||!std::isfinite(r.y)||!std::isfinite(r.width)||!std::isfinite(r.height))return;
    entries_[std::string(name)]={"rect",r,0,enabled};
}
void Handles::count(std::string_view name,double value){if(name_ok(name)&&entries_.size()<16384&&std::isfinite(value))entries_[std::string(name)]={"count",{},value};}
std::optional<std::pair<double,double>> Handles::at(std::string_view name)const{
    auto found=entries_.find(std::string(name));if(found==entries_.end()||found->second.kind=="count")return {};
    const auto r=found->second.rect;return std::pair{double(r.x+r.width*.5f),double(r.y+r.height*.5f)};
}
bool Handles::enabled(std::string_view name)const{auto found=entries_.find(std::string(name));return found!=entries_.end()&&found->second.enabled;}
nlohmann::json Handles::json()const{auto result=nlohmann::json::object();for(const auto& [name,e]:entries_){
    if(e.kind=="count")result[name]={{"kind",e.kind},{"value",e.value}};
    else {result[name]={{"kind",e.kind},{"x",e.rect.x},{"y",e.rect.y},{"enabled",e.enabled}};if(e.kind=="rect"){result[name]["width"]=e.rect.width;result[name]["height"]=e.rect.height;}}
}return result;}
void bind_handles(Handles* registry){current=registry;}
void publish_handle(std::string_view name,double x,double y){if(current)current->point(name,x,y);}
void publish_rect(std::string_view name,LayoutRect rect,bool enabled){if(current)current->rect(name,rect,enabled);}
void publish_count(std::string_view name,double value){if(current)current->count(name,value);}
void publish_item(std::string_view name,bool enabled){if(!ImGui::IsItemVisible())return;const auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();publish_rect(name,{a.x,a.y,b.x-a.x,b.y-a.y},enabled);}
}
