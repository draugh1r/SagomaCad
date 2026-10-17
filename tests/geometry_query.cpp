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
    std::cout<<"Geometry query, filters, exact distance/angle, bridge selection and agent workflow OK\n";
}catch(const std::exception& e){std::cerr<<e.what();return 1;}}
