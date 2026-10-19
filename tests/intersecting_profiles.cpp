#include "engine/command.hpp"
#include "solver/sketch_solver.hpp"
#include "solver/profile_regions.hpp"
#include "kernel/shape.hpp"
#include "regen/regenerator.hpp"
#include "core/planar.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>
using namespace sagomacad;using J=nlohmann::json;
void check(bool b,const std::string& message){if(!b)throw std::runtime_error(message);}
J run(engine::Engine& e,const char* command,J params=J::object()){auto r=e.execute(command,params);check(r.ok,std::string(command)+": "+r.error);return r.value;}
double lens(double a,double b,double d){return a*a*std::acos((d*d+a*a-b*b)/(2*d*a))+b*b*std::acos((d*d+b*b-a*a)/(2*d*b))-.5*std::sqrt((-d+a+b)*(d+a-b)*(d-a+b)*(d+a+b));}
void close(double actual,double expected){check(std::abs(actual-expected)<1e-5,"exact area/volume mismatch: "+std::to_string(actual)+" expected "+std::to_string(expected));}
int main(){try{
    for(const auto plane:{"XY","XZ","YZ","support"}){
        doc::Sketch s;s.plane=plane;s.points={{"a",0,0},{"b",10,0}};s.entities={{"a-circle","circle",{"a"},10},{"b-circle","circle",{"b"},10}};
        if(std::string(plane)=="support"){s.support=doc::TopologyRef{};s.origin={17,31,-4};s.u={0,1,0};s.v={0,0,1};}
        auto regions=solver::profile_regions(s,solver::solve({},s).profiles);check(regions.size()==3,"three exact regions");
        const kernel::Vec3 normal=std::string(plane)=="XY"?kernel::Vec3{0,0,1}:std::string(plane)=="XZ"?kernel::Vec3{0,-1,0}:kernel::Vec3{1,0,0};
        double area=0;for(auto& region:regions){auto face=kernel::profile_face(s,region.boundary,region.holes);check(bool(face),face.error);auto prism=kernel::extrude(*face.value,normal);check(bool(prism),prism.error);auto mass=kernel::mass_properties(*prism.value);check(bool(mass),mass.error);double expected=solver::contains(region,5,0)?lens(10,10,10):100*std::numbers::pi-lens(10,10,10);close(mass.value->volume,expected);area+=mass.value->volume;}
        close(area,200*std::numbers::pi-lens(10,10,10));
    }
    for(double x:{-5.,5.,15.}){
        engine::Engine e;run(e,"param.set",{{"name","diameter"},{"expression","20 mm"}});auto sk=run(e,"sketch.create",{{"plane","XY"}})["sketch"];
        run(e,"sketch.circle",{{"diameter","diameter"}});run(e,"sketch.circle",{{"x",10},{"diameter","20 mm"}});
        auto result=solver::solve(e.session().document,doc::read_sketch(e.session().document.features.at(sk.get<std::string>()).data));auto regions=solver::profile_regions(result.sketch,result.profiles);
        auto selected=std::find_if(regions.begin(),regions.end(),[&](const auto& r){return solver::contains(r,x,0);});check(selected!=regions.end(),"region selectable");
        const auto identity=selected->id;auto extrude=run(e,"feature.extrude",{{"sketch",sk},{"profiles",J::array({identity})},{"distance","20 mm"},{"operation","new"}});
        auto volume=[&](){return run(e,"model.mass_properties",{{"body",extrude["body_id"]}})["volume_mm3"].get<double>();};
        close(volume(),20*(x==5?lens(10,10,10):100*std::numbers::pi-lens(10,10,10)));
        run(e,"param.set",{{"name","diameter"},{"expression","22 mm"}});close(volume(),20*(x==5?lens(11,10,10):(x<0?121:100)*std::numbers::pi-lens(11,10,10)));
        doc::Document loaded;std::string error;check(doc::from_json(doc::to_json(e.session().document),loaded,error),error);e.session().document=loaded;close(volume(),20*(x==5?lens(11,10,10):(x<0?121:100)*std::numbers::pi-lens(11,10,10)));
        auto unrelated=run(e,"primitive.add",{{"kind","box"},{"dimensions",{{"width","2 mm"},{"depth","2 mm"},{"height","2 mm"}}}});
        run(e,"param.set",{{"name","diameter"},{"expression","50 mm"}});auto failed=run(e,"feature.get",{{"feature_id",extrude["feature_id"]}});check(!failed["error"].get<std::string>().empty(),"disappeared region yields a timeline error");close(run(e,"model.mass_properties",{{"body",unrelated["body_id"]}})["volume_mm3"],8);
    }
    // A chord splits a circle into two exact half-discs; dangling line tails
    // are not part of either face. Arc parameters also cross the 2*pi seam.
    doc::Sketch chord;chord.points={{"o",0,0},{"a",-15,0},{"b",15,0}};chord.entities={{"c","circle",{"o"},10},{"line","line",{"a","b"}}};
    auto halves=solver::profile_regions(chord,solver::solve({},chord).profiles);check(halves.size()==2,"line-circle arrangement");
    for(const auto& r:halves){auto face=kernel::profile_face(chord,r.boundary,r.holes);check(bool(face),face.error);auto prism=kernel::extrude(*face.value,{0,0,1});check(bool(prism),prism.error);auto mass=kernel::mass_properties(*prism.value);check(bool(mass),mass.error);close(mass.value->volume,50*std::numbers::pi);}
    chord.entities[0].kind="arc";chord.points.push_back({"start",0,-10});chord.points.push_back({"end",0,10});chord.entities[0].points={"o","start","end"};
    auto quarters=solver::profile_regions(chord,solver::solve({},chord).profiles);check(quarters.empty(),"open arc and chord are not a closed region");
    chord.entities.push_back({"closing","line",{"start","end"}});quarters=solver::profile_regions(chord,solver::solve({},chord).profiles);check(quarters.size()==2,"closed arc split by a crossing line");
    for(const auto& r:quarters){auto face=kernel::profile_face(chord,r.boundary,r.holes);check(bool(face),face.error);auto prism=kernel::extrude(*face.value,{0,0,1});check(bool(prism),prism.error);auto mass=kernel::mass_properties(*prism.value);check(bool(mass),mass.error);close(mass.value->volume,25*std::numbers::pi);}
    // Malformed selectors never resolve to a nearby span.
    auto bad=kernel::profile_face(chord,{"@+missing"},{});check(!bad,"missing analytic span rejected");
    doc::Sketch triple;triple.points={{"a",0,0},{"b",10,0},{"c",5,std::sqrt(75.)}};triple.entities={{"c1","circle",{"a"},10},{"c2","circle",{"b"},10},{"c3","circle",{"c"},10}};
    auto seven=solver::profile_regions(triple,solver::solve({},triple).profiles);check(seven.size()==7,"three intersecting circles yield seven bounded faces");
    double total=0;for(const auto& r:seven){auto face=kernel::profile_face(triple,r.boundary,r.holes);check(bool(face),face.error);auto solid=kernel::extrude(*face.value,{0,0,1});check(bool(solid),solid.error);total+=kernel::mass_properties(*solid.value).value->volume;}
    close(total,300*std::numbers::pi-3*lens(10,10,10)+(std::numbers::pi-std::sqrt(3.))*50);
    for(auto& p:triple.points){double x=p.x,y=p.y;p.x=x*std::cos(.37)-y*std::sin(.37);p.y=x*std::sin(.37)+y*std::cos(.37);}std::reverse(triple.entities.begin(),triple.entities.end());
    auto rotated=solver::profile_regions(triple,solver::solve({},triple).profiles);for(const auto& r:seven)check(std::any_of(rotated.begin(),rotated.end(),[&](const auto& other){return other.id==r.id;}),"IDs survive rotation and entity order changes");
    bool bounded=false;try{core::planar::arrange(std::vector<core::planar::Curve>(2049));}catch(const std::exception&){bounded=true;}check(bounded,"arrangement bounds hostile allocation");
    std::cout<<"Three selectable exact regions: areas, three planes and rotated support; independent extrusion, radius edit, persistence and lost-region errors OK\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
