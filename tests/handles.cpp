#include "shell.hpp"
#include <iostream>
#include <limits>
using namespace sagomacad;
int main(){
    ui::Handles registry;registry.point("p",3,4);registry.rect("r",{10,20,30,40},false);registry.count("n",7);
    if(registry.at("p")!=std::pair{3.,4.}||registry.at("r")!=std::pair{25.,40.}||registry.at("n")||registry.enabled("r"))return 1;
    registry.point("bad",std::numeric_limits<double>::quiet_NaN(),0);if(registry.json().contains("bad"))return 2;
    registry.clear();if(!registry.json().empty())return 3;
    engine::Engine engine;Shell shell(1440,900,1,true,true);if(!shell.valid())return 4;shell.bind_engine(&engine);shell.render();shell.render();
    auto call=[&](const char* method,nlohmann::json params=nlohmann::json::object()){
        auto result=shell.control(method,params);if(result.contains("error"))throw std::runtime_error(result.dump());return result["result"];
    };
    try{
        auto state=call("ui.inspect");if(!state["handles"].contains("menu:CREA")||!state["handles"].contains("cube:top-front")||state["handles"].contains("toolbar:primitive.add"))return 5;
        call("ui.click",{{"handle","menu:CREA"}});if(!call("ui.inspect")["handles"].contains("toolbar:primitive.add"))return 16;
        call("ui.click",{{"handle","toolbar:primitive.add"}});if(!call("ui.inspect")["handles"].contains("field:Larghezza"))return 6;
        call("ui.click",{{"handle","field:Larghezza"}});call("ui.key",{{"key","Ctrl+A"}});call("ui.type",{{"text","50 mm"}});call("ui.click",{{"handle","field:Crea"}});
        if(engine.session().document.bodies.size()!=1)return 7;auto mass=engine.execute("model.mass_properties");if(!mass.ok||std::abs(mass.value["volume_mm3"].get<double>()-40000)>1e-6)return 8;
        auto handles=call("ui.inspect")["handles"];if(handles.contains("field:Crea")||!handles.contains("timeline:primitive-1")||!handles.contains("row:body-1"))return 9;
        if(!shell.control("ui.at",{{"handle","missing"}}).contains("error"))return 10;
        if(!shell.control("ui.click",{{"handle","bodies"}}).contains("error"))return 11;
        auto invalid=shell.control("ui.pointer",{{"events",nlohmann::json::array({{{"kind","down"},{"x",10},{"y",10}},{{"kind","bad"},{"x",10},{"y",10}}})}});if(!invalid.contains("error"))return 12;
        if(!shell.control("ui.frame",{{"seconds",1e-300}}).contains("error"))return 17;
        if(!shell.control("ui.key",{{"key",std::string(1000,'+')}}).contains("error"))return 18;
        if(!shell.control("ui.key",{{"key","A"},{"modifiers",{"Ctrl","bad"}}}).contains("error"))return 19;
        if(!shell.control("ui.click",{{"x",10},{"y",10},{"button",4294967296ULL}}).contains("error"))return 20;
        call("ui.resize",{{"width",800},{"height",600}});state=call("ui.inspect");if(state["width"]!=800||state["height"]!=600)return 13;
        call("ui.set",{{"panels",{{"browser",false}}}});if(call("ui.inspect")["handles"].contains("row:body-1"))return 14;
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what();return 15;}
}
