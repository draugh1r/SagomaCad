#include "shell.hpp"
#include "automation/dispatch.hpp"
#include "ui/sketch_view.hpp"
#include <SDL3/SDL.h>
#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <cctype>
#include <stdexcept>
namespace sagomacad {
namespace {
using J=nlohmann::json;
double number(const J& p,const char* key,double limit=1e7){
    if(!p.contains(key)||!p.at(key).is_number())throw std::runtime_error(std::string(key)+" must be a number");
    double value=p.at(key).get<double>();if(!std::isfinite(value)||std::abs(value)>limit)throw std::runtime_error(std::string(key)+" out of range");return value;
}
int integer(const J& p,const char* name,int fallback,int minimum,int maximum){
    if(!p.contains(name))return fallback;const auto& value=p.at(name);
    if(!value.is_number_integer()||value<minimum||value>maximum)throw std::runtime_error(std::string(name)+" must be an integer in range");return value.get<int>();
}
ImGuiKey key_code(std::string key){
    if(key.size()==1){char c=char(std::toupper(static_cast<unsigned char>(key[0])));if(c>='A'&&c<='Z')return ImGuiKey(ImGuiKey_A+c-'A');if(c>='0'&&c<='9')return ImGuiKey(ImGuiKey_0+c-'0');}
    for(auto [name,code]:{std::pair{"Enter",ImGuiKey_Enter},{"Tab",ImGuiKey_Tab},{"Escape",ImGuiKey_Escape},{"Esc",ImGuiKey_Escape},{"Space",ImGuiKey_Space},{"Delete",ImGuiKey_Delete},{"Backspace",ImGuiKey_Backspace},{"Left",ImGuiKey_LeftArrow},{"Right",ImGuiKey_RightArrow},{"Up",ImGuiKey_UpArrow},{"Down",ImGuiKey_DownArrow},{"Home",ImGuiKey_Home},{"End",ImGuiKey_End}})if(key==name)return code;
    throw std::runtime_error("Unsupported key: "+key);
}
void modifiers(const J& names,bool down){
    if(!names.is_array()||names.size()>4)throw std::runtime_error("modifiers must be an array");
    std::vector<ImGuiKey> keys;
    for(const auto& name:names){if(!name.is_string())throw std::runtime_error("modifier must be a string");auto s=name.get<std::string>();ImGuiKey key;
        if(s=="Ctrl"||s=="Control")key=ImGuiMod_Ctrl;else if(s=="Shift")key=ImGuiMod_Shift;else if(s=="Alt")key=ImGuiMod_Alt;else if(s=="Super")key=ImGuiMod_Super;else throw std::runtime_error("Unknown modifier: "+s);
        keys.push_back(key);
    }
    for(auto key:keys)ImGui::GetIO().AddKeyEvent(key,down);
}
}
J Shell::control(std::string_view method,const J& p){
    try {
        if(!p.is_object())throw std::runtime_error("params must be an object");
        auto done=[&](){return J{{"result",J::object()}};};
        auto frame=[&](double seconds=1./60){poll();render(seconds);};
        auto settle=[&](){frame();frame();};
        auto at=[&](const J& point)->std::pair<double,double>{
            if(point.contains("handle")){
                if(!point["handle"].is_string())throw std::runtime_error("handle must be a string");
                auto name=point["handle"].get<std::string>();auto found=handles_.at(name);if(!found)throw std::runtime_error("Unknown or non-positional handle: "+name);return *found;
            }
            if(point.contains("browser")){
                if(!point["browser"].is_string())throw std::runtime_error("browser must be a string");auto found=handles_.at("row:"+point["browser"].get<std::string>());if(!found)throw std::runtime_error("Unknown browser row");return *found;
            }
            kernel::Vec3 world;
            if(point.contains("world")){
                const auto& values=point["world"];if(!values.is_array()||values.size()!=3)throw std::runtime_error("world requires three numbers");
                J coordinates{{"x",values[0]},{"y",values[1]},{"z",values[2]}};world={number(coordinates,"x"),number(coordinates,"y"),number(coordinates,"z")};
            }else if(point.contains("sketch")){
                const auto& values=point["sketch"];if(!values.is_array()||values.size()!=2)throw std::runtime_error("sketch requires two numbers");
                auto found=sketch_solutions_.find(state_.sketch_view_id);if(found==sketch_solutions_.end())throw std::runtime_error("No active sketch plane");
                J xy{{"x",values[0]},{"y",values[1]}};world=ui::sketch_world(found->second.sketch,{number(xy,"x"),number(xy,"y")});
            }else if(point.contains("plane")){
                auto plane=point.at("plane").get<std::string>();if(plane!="XY"&&plane!="XZ"&&plane!="YZ")throw std::runtime_error("Unknown plane");
                const double extent=ui::theme(state_.theme=="light").sketch.plane_extent*.65;world=ui::sketch_world(plane,{extent,extent});
            }else return {number(point,"x"),number(point,"y")};
            auto q=state_.camera.project(world,current_viewport_);if(!q.visible||!std::isfinite(q.x)||!std::isfinite(q.y))throw std::runtime_error("Point is behind the camera");return {q.x,q.y};
        };
        if(method=="engine.execute"||method=="engine.commands"||method=="engine.batch"){
            if(!engine_)throw std::runtime_error("No engine");auto result=automation::dispatch(*engine_,method,p);if(!result.contains("error"))settle();return result;
        }
        if(method=="ui.inspect")return {{"result",inspect()}};
        if(method=="ui.at"){auto [x,y]=at(p);return {{"result",{{"x",x},{"y",y}}}};}
        if(method=="ui.frame"){
            const double seconds=p.contains("seconds")?number(p,"seconds",1):1./60;if(seconds<1e-6)throw std::runtime_error("seconds must be positive");frame(seconds);return done();
        }
        if(method=="ui.click"){
            auto [x,y]=at(p);const int button=integer(p,"button",0,0,2),count=integer(p,"count",1,1,2);
            if(button<0||button>2||count<1||count>2)throw std::runtime_error("Invalid button or click count");
            if(p.contains("handle")&&!handles_.enabled(p.at("handle").get<std::string>()))throw std::runtime_error("Handle is disabled");
            auto mods=p.value("modifiers",J::array());modifiers(mods,true);auto& io=ImGui::GetIO();control_pointer_=std::pair{float(x),float(y)};io.AddMousePosEvent(float(x),float(y));frame();
            for(int i=0;i<count;++i){control_buttons_[button]=true;io.AddMouseButtonEvent(button,true);frame();control_buttons_[button]=false;io.AddMouseButtonEvent(button,false);frame();}
            modifiers(mods,false);settle();control_pointer_.reset();return done();
        }
        if(method=="ui.pointer"){
            const auto& events=p.at("events");if(!events.is_array()||events.size()>4096)throw std::runtime_error("events must be an array with at most 4096 items");
            for(const auto& e:events){at(e);auto kind=e.at("kind").get<std::string>();const int button=integer(e,"button",0,0,2);if(button<0||button>2||(kind!="down"&&kind!="up"&&kind!="move"))throw std::runtime_error("Invalid pointer event");}
            auto mods=p.value("modifiers",J::array());modifiers(mods,true);
            for(const auto& e:events){auto [x,y]=at(e);auto kind=e.at("kind").get<std::string>();int button=integer(e,"button",0,0,2);if(button<0||button>2)throw std::runtime_error("Invalid button");
                control_pointer_=std::pair{float(x),float(y)};ImGui::GetIO().AddMousePosEvent(float(x),float(y));if(kind=="down"||kind=="up"){control_buttons_[button]=kind=="down";ImGui::GetIO().AddMouseButtonEvent(button,kind=="down");}else if(kind!="move")throw std::runtime_error("Unknown pointer event");frame();}
            modifiers(mods,false);settle();if(std::none_of(control_buttons_.begin(),control_buttons_.end(),[](bool value){return value;}))control_pointer_.reset();return done();
        }
        if(method=="ui.key"){
            auto key=p.at("key").get<std::string>();if(key.size()>64)throw std::runtime_error("key too long");auto mods=p.value("modifiers",J::array());
            for(auto pos=key.find('+');pos!=std::string::npos;pos=key.find('+')){mods.push_back(key.substr(0,pos));key.erase(0,pos+1);}
            const auto code=key_code(key);modifiers(mods,true);ImGui::GetIO().AddKeyEvent(code,true);frame();ImGui::GetIO().AddKeyEvent(code,false);modifiers(mods,false);settle();return done();
        }
        if(method=="ui.type"){
            const auto text=p.at("text").get<std::string>();if(text.size()>4096)throw std::runtime_error("text too long");ImGui::GetIO().AddInputCharactersUTF8(text.c_str());settle();return done();
        }
        if(method=="ui.resize"){
            const auto w=number(p,"width",4096),h=number(p,"height",4096);if(w<320||h<240||std::floor(w)!=w||std::floor(h)!=h)throw std::runtime_error("Invalid window dimensions");
            if(!SDL_SetWindowSize(window_,int(w)*scale_,int(h)*scale_))throw std::runtime_error(SDL_GetError());state_.width=int(w);state_.height=int(h);settle();return done();
        }
        if(method=="ui.set"){
            // Apply explicit camera values after the current document's first automatic fit.
            frame();auto candidate=state_.camera;
            for(auto it=p.begin();it!=p.end();++it)if(it.key()!="camera"&&it.key()!="view"&&it.key()!="theme"&&it.key()!="panels"&&it.key()!="tool")throw std::runtime_error("Unknown ui.set field: "+it.key());
            if(p.contains("theme")){auto theme=p.at("theme").get<std::string>();if(theme!="light"&&theme!="dark")throw std::runtime_error("Unknown theme");state_.theme=theme;}
            if(p.contains("view")){
                auto view=p.at("view").get<std::string>();if(view=="iso")candidate.home();else if(view=="top")candidate.look_from({0,0,1});else if(view=="bottom")candidate.look_from({0,0,-1});else if(view=="front")candidate.look_from({0,-1,0});else if(view=="back")candidate.look_from({0,1,0});else if(view=="left")candidate.look_from({-1,0,0});else if(view=="right")candidate.look_from({1,0,0});else throw std::runtime_error("Unknown view");
            }
            if(p.contains("camera")){
                const auto& c=p.at("camera");if(!c.is_object())throw std::runtime_error("camera must be an object");
                for(auto it=c.begin();it!=c.end();++it){
                    if(it.key()=="target"){if(!it->is_array()||it->size()!=3)throw std::runtime_error("target requires three numbers");J t{{"x",(*it)[0]},{"y",(*it)[1]},{"z",(*it)[2]}};candidate.target={number(t,"x"),number(t,"y"),number(t,"z")};}
                    else if(it.key()=="yaw")candidate.yaw=number(c,"yaw");else if(it.key()=="pitch")candidate.pitch=number(c,"pitch");else if(it.key()=="roll")candidate.roll=number(c,"roll");else if(it.key()=="distance"){candidate.distance=number(c,"distance");if(candidate.distance<.05)throw std::runtime_error("distance too small");}else throw std::runtime_error("Unknown camera field");
                }
            }
            if(p.contains("panels")&&!p.at("panels").is_object())throw std::runtime_error("panels must be an object");
            if(p.contains("panels"))for(auto it=p.at("panels").begin();it!=p.at("panels").end();++it){if(!it->is_boolean())throw std::runtime_error("Panel visibility must be boolean");if(it.key()=="browser")state_.browser=*it;else if(it.key()=="properties")state_.properties=*it;else if(it.key()=="timeline")state_.timeline=*it;else throw std::runtime_error("Unknown panel");}
            if(p.contains("tool")){auto tool=p.at("tool").get<std::string>();if(tool.starts_with("sketch."))tool.erase(0,7);if(tool!="line"&&tool!="rect"&&tool!="circle"&&tool!="arc"&&tool!="select"&&tool!="dimension")throw std::runtime_error("Unknown sketch tool");if(!state_.sketch_mode)throw std::runtime_error("No active sketch");state_.sketch_tool.begin(tool);}
            if(p.contains("camera")||p.contains("view")){state_.view_transition.cancel();state_.camera=candidate;}settle();return done();
        }
        throw std::runtime_error("Unknown UI method: "+std::string(method));
    }catch(const std::exception& e){return {{"error",{{"message",e.what()}}}};}
}
}
