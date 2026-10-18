#include "engine/command.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace sagomacad;using J=nlohmann::json;
void check(bool b,const std::string& m){if(!b)throw std::runtime_error(m);}
J run(engine::Engine& e,const char* id,J p=J::object()){auto r=e.execute(id,p);check(r.ok,std::string(id)+": "+r.error);return r.value;}
void close(double a,double b){check(std::abs(a-b)<1e-4,"unexpected volume or bounds");}
int main(){try{
    for(auto direction:{"one_side","symmetric","two_sides"})for(auto operation:{"new","join","cut","intersect"}){
        engine::Engine e;auto b=run(e,"primitive.add",{{"kind","box"},{"dimensions",{{"width","40 mm"},{"depth","30 mm"},{"height","20 mm"}}}});
        J ref={{"body",b["body_id"]},{"name",b["feature_id"].get<std::string>()+"/z_max"}};
        double distance=std::string(operation)=="cut"||std::string(operation)=="intersect"?-5:5;
        if(std::string(direction)=="two_sides")distance=5;
        J p={{"face",ref},{"distance",std::to_string(distance)+" mm"},{"second_distance","3 mm"},{"direction",direction},{"operation",operation}};
        if(std::string(operation)!="new")p["target_bodies"]=J::array({b["body_id"]});
        auto f=run(e,"feature.extrude",p);auto mass=run(e,"model.mass_properties",{{"body",f["body_id"]}});
        double swept=std::string(direction)=="two_sides"?8:5;
        double overlap=std::string(direction)=="two_sides"?3:std::string(direction)=="symmetric"?2.5:distance<0?5:0;
        double expected=std::string(operation)=="new"?1200*swept:std::string(operation)=="join"?24000+1200*(swept-overlap):std::string(operation)=="cut"?24000-1200*overlap:1200*overlap;
        close(mass["volume_mm3"],expected);
        run(e,"feature.edit",{{"feature_id",b["feature_id"]},{"dimensions",{{"height","35 mm"},{"width","60 mm"}}}});
        close(run(e,"model.mass_properties",{{"body",f["body_id"]}})["volume_mm3"],std::string(operation)=="new"?1800*swept:std::string(operation)=="join"?63000+1800*(swept-overlap):std::string(operation)=="cut"?63000-1800*overlap:1800*overlap);
    }
    engine::Engine e;auto b=run(e,"primitive.add",{{"kind","box"},{"dimensions",{{"width","10 mm"},{"depth","20 mm"},{"height","30 mm"}}}});
    auto ref=J{{"body",b["body_id"]},{"name",b["feature_id"].get<std::string>()+"/x_max"}};
    auto f=run(e,"feature.extrude",{{"face",ref},{"distance","7 mm"},{"operation","new"}});auto m=run(e,"model.mass_properties",{{"body",f["body_id"]}});close(m["volume_mm3"],4200);close(m["bbox_mm"]["min"]["x"],10);
    run(e,"feature.edit",{{"feature_id",f["feature_id"]},{"params",{{"distance","9 mm"}}}});close(run(e,"model.mass_properties",{{"body",f["body_id"]}})["volume_mm3"],5400);
    auto before=e.session().document;check(!e.execute("feature.extrude",{{"face",ref},{"sketch","bad"}}).ok&&e.session().document==before,"ambiguous source atomic error");
    for(auto direction:{"one_side","symmetric"}){
        engine::Engine through;auto source=run(through,"primitive.add",{{"kind","box"},{"dimensions",{{"width","10 mm"},{"depth","20 mm"},{"height","30 mm"}}}});
        auto target=run(through,"primitive.add",{{"kind","box"},{"dimensions",{{"width","50 mm"},{"depth","50 mm"},{"height","40 mm"}}}});
        run(through,"feature.extrude",{{"face",{{"body",source["body_id"]},{"name",source["feature_id"].get<std::string>()+"/z_max"}}},{"distance","-1 mm"},{"direction",direction},{"extent","through_all"},{"operation","cut"},{"target_bodies",J::array({target["body_id"]})}});
        close(run(through,"model.mass_properties",{{"body",target["body_id"]}})["volume_mm3"],std::string(direction)=="one_side"?94000:92000);
    }
    std::cout<<"Face extrusion: 12 operation/direction combinations, upstream edits, side face normal and feature edit OK\n";
}catch(const std::exception& e){std::cerr<<e.what();return 1;}}
