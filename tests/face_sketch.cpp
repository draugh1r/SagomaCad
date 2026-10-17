#include "engine/command.hpp"
#include "regen/attached_sketch.hpp"
#include "kernel/topology.hpp"
#include <cmath>
#include <numbers>
#include <iostream>
#include <stdexcept>
using namespace sagomacad;
using J=nlohmann::json;
void check(bool b,const std::string& m){if(!b)throw std::runtime_error(m);}
J run(engine::Engine& e,const char* id,J p=J::object()){auto r=e.execute(id,p);check(r.ok,std::string(id)+": "+r.error);return r.value;}
void close(double a,double b){check(std::abs(a-b)<1e-4,std::to_string(a)+" != "+std::to_string(b));}
J base(engine::Engine& e){return run(e,"primitive.add",{{"kind","box"},{"dimensions",{{"width","40 mm"},{"depth","40 mm"},{"height","20 mm"}}}});}
int main(){try{
    engine::Engine e;auto b=base(e);auto ref=J{{"body",b["body_id"]},{"name",b["feature_id"].get<std::string>()+"/z_max"}};
    auto s=run(e,"sketch.create",{{"face",ref}})["sketch"].get<std::string>();auto before=run(e,"sketch.inspect");
    check(before["entities"].size()==4&&before["degrees_of_freedom"]==0,"four fixed reference edges");
    for(const auto& entity:before["entities"])check(entity["construction"]==true&&entity.contains("reference_name"),"reference metadata");
    check(!e.execute("sketch.point.move",{{"point",before["points"][0]["id"]},{"x",7},{"y",9}}).ok,"projection immutable");
    auto circle=run(e,"sketch.circle",{{"x",10},{"y",10},{"diameter","10 mm"}});auto profiles=run(e,"sketch.inspect")["regions"];check(profiles.size()==1,"projection not a profile");
    auto cylinder=run(e,"feature.extrude",{{"sketch",s},{"profiles",J::array({profiles[0]["id"]})},{"distance","6 mm"}});
    run(e,"feature.edit",{{"feature_id",b["feature_id"]},{"dimensions",{{"height","35 mm"},{"width","60 mm"}}}});
    auto m=run(e,"model.mass_properties",{{"body",cylinder["body_id"]}});close(m["bbox_mm"]["min"]["z"],35);close(m["volume_mm3"],150*std::numbers::pi);
    auto updated=run(e,"sketch.inspect",{{"sketch",s}});close(updated["origin"][2],35);
    auto roundtrip=doc::to_json(e.session().document);doc::Document loaded;std::string error;check(doc::from_json(roundtrip,loaded,error),error);e.session().document=loaded;
    close(run(e,"model.mass_properties",{{"body",cylinder["body_id"]}})["bbox_mm"]["min"]["z"],35);
    // Constraints may refer to projected geometry and follow its upstream movement.
    run(e,"sketch.edit",{{"sketch",s}});auto loose=run(e,"sketch.circle",{{"diameter","4 mm"},{"anchor",false}});auto point=updated["entities"][0]["points"][0];
    run(e,"sketch.constraint.add",{{"kind","coincident"},{"refs",J::array({loose["center"],point})}});
    run(e,"feature.edit",{{"feature_id",b["feature_id"]},{"dimensions",{{"width","80 mm"}}}});
    auto constrained=run(e,"sketch.inspect",{{"sketch",s}});check(constrained["state"]=="constrained","reference participates in constraints");
    engine::Engine gone;auto gb=base(gone);auto cut=run(gone,"primitive.add",{{"kind","box"},{"operation","subtract"},{"body",gb["body_id"]},{"position",{{"z","10 mm"}}},{"dimensions",{{"width","5 mm"},{"depth","40 mm"},{"height","10 mm"}}}});
    auto gs=run(gone,"sketch.create",{{"face",{{"body",gb["body_id"]},{"name",gb["feature_id"].get<std::string>()+"/z_max"}}}})["sketch"];
    run(gone,"sketch.circle",{{"x",15},{"y",15},{"diameter","4 mm"}});auto region=run(gone,"sketch.inspect")["regions"][0]["id"];
    auto gf=run(gone,"feature.extrude",{{"sketch",gs},{"profiles",J::array({region})},{"distance","5 mm"}});
    run(gone,"feature.edit",{{"feature_id",cut["feature_id"]},{"dimensions",{{"width","40 mm"}}}});
    auto failed=run(gone,"feature.get",{{"feature_id",gs}});check(failed["error"].get<std::string>().find("eliminata")!=std::string::npos,"deleted support readable error");
    check(!run(gone,"feature.get",{{"feature_id",gf["feature_id"]}})["error"].get<std::string>().empty(),"dependent extrusion error");
    run(gone,"primitive.add",{{"kind","sphere"},{"dimensions",{{"radius","2 mm"}}}});auto snapshot=regen::Regenerator{}.evaluate(gone.session().document);check(snapshot.features.back().error.empty(),"regen continues after reference failure");
    std::cout<<"Face sketches: immutable projections, constraints, upstream frame, cylinder stays on top, persistence, deleted support and continued regen OK\n";
}catch(const std::exception& e){std::cerr<<e.what();return 1;}}
