#include "ui/model_editor.hpp"
#include "engine/tools/extrusion_defaults.hpp"
#include "regen/regenerator.hpp"
#include "kernel/topology.hpp"
#include <cmath>
#include <cstdio>
#include <iostream>
#include <stdexcept>
using namespace sagomacad;using J=nlohmann::json;
void check(bool b,const std::string& m){if(!b)throw std::runtime_error(m);}
J run(engine::Engine& e,const char* id,J p=J::object()){auto r=e.execute(id,p);check(r.ok,std::string(id)+": "+r.error);return r.value;}
void close(double a,double b){check(std::abs(a-b)<1e-4,"GProp expected "+std::to_string(b)+", got "+std::to_string(a));}
int main(){try{
    for(const auto* face:{"x_max","x_min","y_max","y_min","z_max","z_min"})for(const auto* distance:{"5 mm","-5 mm"}){
        engine::Engine e;auto b=run(e,"primitive.add",{{"kind","box"},{"dimensions",{{"width","10 mm"},{"depth","20 mm"},{"height","30 mm"}}}});
        J ref={{"body",b["body_id"]},{"name",b["feature_id"].get<std::string>()+"/"+face}};
        auto before=e.session().document;auto suggestion=run(e,"feature.extrude.suggest",{{"face",ref},{"distance",distance}});
        const bool cut=distance[0]=='-';check(suggestion["operation"]==(cut?"cut":"join"),"oriented face automatic operation");check(e.session().document==before,"suggestion read only");
        auto f=run(e,"feature.extrude",{{"face",ref},{"distance",distance}});
        check(e.session().document.bodies.size()==1&&f["body_id"]==b["body_id"],"one body after face extrusion");
        auto mass=run(e,"model.mass_properties",{{"body",b["body_id"]}});
        double w=10,h=30,d=20;auto& side=face[0]=='x'?w:face[0]=='y'?d:h;side+=cut?-5:5;
        close(mass["volume_mm3"],w*d*h);close(mass["area_mm2"],2*(w*d+w*h+d*h)); // Internal coincident faces would increase area.
        run(e,"edit.undo");ui::State state;state.model.open=true;state.model.face=ref;std::snprintf(state.model.second_distance.data(),state.model.second_distance.size(),"10 mm");std::snprintf(state.model.distance.data(),state.model.distance.size(),"5 mm");
        ui::update_model_operation(state,e);check(state.model.operation==0,"UI joins");
        std::snprintf(state.model.distance.data(),state.model.distance.size(),"-5 mm");ui::update_model_operation(state,e);check(state.model.operation==1,"UI follows sign");
        state.model.operation=3;state.model.operation_manual=true;std::snprintf(state.model.distance.data(),state.model.distance.size(),"8 mm");ui::update_model_operation(state,e);check(state.model.operation==3,"manual override stays fixed");
        check(ui::submit_model(state,e),state.model.error);check(e.session().document.bodies.size()==2,"explicit new body honored");
    }
    for(double x:{0.,40.}){
        engine::Engine e;auto b=run(e,"primitive.add",{{"kind","box"},{"dimensions",{{"width","10 mm"},{"depth","20 mm"},{"height","30 mm"}}}});
        auto sk=run(e,"sketch.create",{{"plane","XY"}})["sketch"];auto r=run(e,"sketch.rect",{{"x",x},{"width","8 mm"},{"height","8 mm"}});run(e,"sketch.finish");
        J params={{"sketch",sk},{"profiles",J::array({r["entities"]})},{"distance","-5 mm"}};
        check(run(e,"feature.extrude.suggest",params)["operation"]==(x==0?"join":"new"),"profile contact, outward negative on XY bottom");
        params["distance"]="5 mm";check(run(e,"feature.extrude.suggest",params)["operation"]==(x==0?"cut":"new"),"profile entering material or detached");
        params["operation"]="new";run(e,"feature.extrude",params);check(e.session().document.bodies.size()==2,"command manual override");
    }
    std::cout<<"M5: 12 oriented face cases, GProp volume and external area, contact profiles, sign and override OK\n";
}catch(const std::exception& e){std::cerr<<e.what();return 1;}}
