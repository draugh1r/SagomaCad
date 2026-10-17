#include "engine/command.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace sagomacad;using J=nlohmann::json;
void check(bool b,const std::string& m){if(!b)throw std::runtime_error(m);}
J run(engine::Engine& e,const char* id,J p=J::object()){auto r=e.execute(id,p);check(r.ok,std::string(id)+": "+r.error);return r.value;}
int main(){try{
    engine::Engine e;run(e,"primitive.add",{{"kind","box"},{"dimensions",{{"width","40 mm"},{"depth","30 mm"},{"height","20 mm"}}}});
    auto all=run(e,"geometry.query")["entities"];check(all.size()==26,"all entities");auto vertical=run(e,"geometry.query",{{"kind","edge"},{"type","line"},{"parallel_to",{0,0,1}}})["entities"];check(vertical.size()==4,"vertical edges filter");J refs=J::array();for(const auto& row:vertical){check(!row["name"].get<std::string>().empty()&&row["ref"]["signature"]["measure"]==20,"stable names and geometric description");refs.push_back(row["ref"]);}
    auto top=run(e,"geometry.query",{{"kind","face"},{"normal",{0,0,1}}})["entities"][0]["ref"],bottom=run(e,"geometry.query",{{"kind","face"},{"normal",{0,0,-1}}})["entities"][0]["ref"];
    auto measure=run(e,"inspect.measure",{{"a",top},{"b",bottom}});check(std::abs(measure["distance_mm"].get<double>()-20)<1e-6&&measure["angle_degrees"]==0,"face distance and angle");
    auto side=run(e,"geometry.query",{{"kind","face"},{"normal",{1,0,0}}})["entities"][0]["ref"];check(run(e,"inspect.measure",{{"a",top},{"b",side}})["angle_degrees"]==90,"perpendicular planes");
    check(!e.execute("selection.get").ok,"selection unavailable headless");e.session().native_view=true;run(e,"selection.set",{{"refs",J::array({top})}});check(run(e,"selection.get")["refs"].size()==1,"native selection");
    run(e,"feature.fillet",{{"edges",refs},{"radius","2 mm"}});check(run(e,"geometry.query",{{"kind","face"}})["entities"].size()==10,"agent fillets all vertical edges");
    auto limited=run(e,"geometry.query",{{"limit",2}});check(limited["entities"].size()==2&&limited["truncated"]==true,"query allocation limit");
    check(!e.execute("geometry.query",{{"parallel_to",{0,0,0}}}).ok,"invalid direction rejected");
    engine::Engine swept;run(swept,"param.set",{{"name","width"},{"expression","10 mm"}});auto sk=run(swept,"sketch.create",{{"plane","XZ"}})["sketch"];auto rect=run(swept,"sketch.rect",{{"x",5},{"width","width"},{"height","20 mm"}});
    auto revolved=run(swept,"feature.revolve",{{"sketch",sk},{"profiles",J::array({rect["entities"]})},{"axis","Z"},{"type","full"}});
    auto cylinders=run(swept,"geometry.query",{{"body",revolved["body_id"]},{"kind","face"},{"type","cylinder"}})["entities"];check(cylinders.size()==2,"inner and outer revolving faces");
    J cylinder_refs=J::array();for(const auto& row:cylinders){check(row["ambiguous"]==false,"revolve side identities are unique");cylinder_refs.push_back(row["ref"]);}
    run(swept,"selection.set",{{"refs",cylinder_refs}});run(swept,"param.set",{{"name","width"},{"expression","15 mm"}});run(swept,"selection.set",{{"refs",cylinder_refs}});
    engine::Engine prism;auto ps=run(prism,"sketch.create",{{"plane","XY"}})["sketch"];auto pr=run(prism,"sketch.rect",{{"width","10 mm"},{"height","20 mm"}});auto pf=run(prism,"feature.extrude",{{"sketch",ps},{"profiles",J::array({pr["entities"]})},{"distance","5 mm"}});
    auto prism_faces=run(prism,"geometry.query",{{"kind","face"}})["entities"];int sides=0;for(const auto& row:prism_faces)if(row["name"].get<std::string>().find("/side(entity(")!=std::string::npos)++sides;check(sides==4,"all four prism sides preserve sketch entity identity");
    std::cout<<"Geometry query, filters, exact distance/angle, bridge selection and agent workflow OK\n";
}catch(const std::exception& e){std::cerr<<e.what();return 1;}}
