#include "engine/command.hpp"
#include "kernel/topology.hpp"
#include "regen/regenerator.hpp"
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>
using namespace sagomacad;using J=nlohmann::json;
void check(bool b,const std::string& m){if(!b)throw std::runtime_error(m);}
template<class T>T ok(kernel::Result<T> r){check(bool(r),r.error);return *r.value;}
void close(double a,double b){check(std::abs(a-b)<1e-5,"GProp "+std::to_string(a)+" != "+std::to_string(b));}
int faces(const kernel::Shape& shape,const std::string& type={}){int count=0;for(const auto& r:ok(kernel::topology(shape)))if(r.signature.kind=="face"&&(type.empty()||type==r.signature.type))++count;return count;}
J run(engine::Engine& e,const char* id,J p=J::object()){auto r=e.execute(id,p);check(r.ok,std::string(id)+": "+r.error);return r.value;}
int main(){try{
    const auto base=ok(kernel::name_topology(ok(kernel::make_box(20,20,10)),"base"));
    const auto extension=ok(kernel::name_topology(ok(kernel::translate(ok(kernel::make_box(20,20,10)),20,0,0)),"extension"));
    auto bar=ok(kernel::boolean_operation(base,extension,"join"));
    close(ok(kernel::mass_properties(bar)).volume,8000);check(faces(bar)==6,"coplanar box join must have 6 faces");
    auto top=ok(kernel::resolve(bar,{"body","base/z_max"},"face"));auto other=ok(kernel::resolve(bar,{"body","extension/z_max"},"face"));check(top.id==other.id,"both pre-unify aliases resolve to merged face");
    int merged_edges_checked=0;
    for(const auto& edge:ok(kernel::topology(base)))if(edge.signature.kind=="edge"&&std::abs(edge.signature.center[0]-10)<1e-7&&std::abs(edge.signature.center[1])<1e-7&&std::abs(edge.signature.center[2]-10)<1e-7){
        ++merged_edges_checked;
        auto merged=ok(kernel::resolve(bar,{"body",edge.name,edge.signature},"edge"));close(merged.signature.measure,40);close(merged.signature.center[0],20);
    }
    check(merged_edges_checked==1,"pre-unify edge alias was actually checked");
    auto gone=kernel::resolve(bar,{"body","base/x_max"},"face");check(!gone&&gone.error.find("eliminata")!=std::string::npos,"internal join face remains deleted");
    auto cylinder=ok(kernel::name_topology(ok(kernel::make_cylinder(5,10)),"cylinder"));auto upper=ok(kernel::name_topology(ok(kernel::translate(ok(kernel::make_cylinder(5,10)),0,0,10)),"upper"));
    auto joined=ok(kernel::boolean_operation(cylinder,upper,"join"));check(faces(joined)==3&&faces(joined,"cylinder")==1,"coaxial cylinders merge their wall");close(ok(kernel::mass_properties(joined)).volume,500*std::numbers::pi);
    auto first_cutter=ok(kernel::translate(ok(kernel::make_cylinder(2,10)),20,10,5));auto second_cutter=ok(kernel::translate(ok(kernel::make_cylinder(2,10)),20,10,-5));
    auto through=ok(kernel::subtract(ok(kernel::subtract(bar,first_cutter)),second_cutter));
    check(faces(through)==7&&faces(through,"cylinder")==1,"through cut has one cylindrical wall and unsplit planar faces");auto mass=ok(kernel::mass_properties(through));close(mass.volume,8000-40*std::numbers::pi);close(mass.area,2800+32*std::numbers::pi);
    auto clip=ok(kernel::translate(ok(kernel::make_box(30,30,20)),5,-5,-5));auto common=ok(kernel::boolean_operation(bar,clip,"intersect"));check(faces(common)==6,"intersection has 6 unified faces");close(ok(kernel::mass_properties(common)).volume,6000);

    engine::Engine e;auto b=run(e,"primitive.add",{{"kind","box"},{"dimensions",{{"width","20 mm"},{"depth","20 mm"},{"height","10 mm"}}}});auto body=b["body_id"];
    auto side=run(e,"feature.extrude",{{"face",{{"body",body},{"name",b["feature_id"].get<std::string>()+"/x_max"}}},{"distance","20 mm"}});
    check(e.session().document.bodies.size()==1&&side["body_id"]==body,"automatic join keeps one document body");
    auto snapshot=regen::Regenerator{}.evaluate(e.session().document);check(faces(*snapshot.bodies[0].exact)==6,"side extrusion: 6 faces");close(ok(kernel::mass_properties(*snapshot.bodies[0].exact)).volume,8000);
    auto sk=run(e,"sketch.create",{{"face",{{"body",body},{"name",b["feature_id"].get<std::string>()+"/z_max"}}}})["sketch"];
    auto rect=run(e,"sketch.rect",{{"width","20 mm"},{"height","20 mm"}});
    auto up=run(e,"feature.extrude",{{"sketch",sk},{"profiles",J::array({rect["entities"]})},{"distance","10 mm"}});
    check(e.session().document.bodies.size()==1&&up["body_id"]==body,"L remains one body");snapshot=regen::Regenerator{}.evaluate(e.session().document);check(faces(*snapshot.bodies[0].exact)==8,"L must have 8 faces");mass=ok(kernel::mass_properties(*snapshot.bodies[0].exact));close(mass.volume,12000);close(mass.area,3600);
    const auto edges=run(e,"geometry.query",{{"body",body},{"kind","edge"},{"parallel_to",{0,1,0}}});J selected;
    for(const auto& edge:edges["entities"]){auto center=edge["geometry"]["center"];if(std::abs(center[0].get<double>()-40)<1e-7&&std::abs(center[2].get<double>()-10)<1e-7)selected=edge["ref"];}
    check(selected.is_object(),"outer L edge found by geometry query");auto rounded=run(e,"feature.fillet",{{"edges",J::array({selected})},{"radius","2 mm"}});
    close(run(e,"model.mass_properties",{{"body",body}})["volume_mm3"],12000-20*(4-std::numbers::pi));
    run(e,"feature.edit",{{"feature_id",side["feature_id"]},{"params",{{"distance","30 mm"}}}});
    check(run(e,"feature.get",{{"feature_id",rounded["feature_id"]}})["error"]=="","fillet survives upstream distance change");
    close(run(e,"model.mass_properties",{{"body",body}})["volume_mm3"],14000-20*(4-std::numbers::pi));
    auto context=e.session().document;--context.rollback;snapshot=regen::Regenerator{}.evaluate(context);auto resolved=ok(kernel::resolve(*snapshot.bodies[0].exact,selected.get<doc::TopologyRef>(),"edge"));close(resolved.signature.center[0],50);close(resolved.signature.center[2],10);close(resolved.signature.measure,20);
    check(run(e,"feature.get",{{"feature_id",sk}})["error"]=="","sketch on merged face survives upstream edit");
    auto serialized=doc::to_json(e.session().document);doc::Document restored;std::string error;check(doc::from_json(serialized,restored,error),error);e.session().document=restored;close(run(e,"model.mass_properties",{{"body",body}})["volume_mm3"],14000-20*(4-std::numbers::pi));
    std::cout<<"Unified booleans: box 1 body/6 faces/8000; L 1 body/8 faces/12000; cylinders, through cut, intersection, aliases and moving fillet OK\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
