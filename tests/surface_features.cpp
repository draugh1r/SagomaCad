#include "engine/command.hpp"
#include "kernel/topology.hpp"
#include "regen/regenerator.hpp"
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>
using namespace sagomacad;using J=nlohmann::json;
void check(bool b,const std::string& m){if(!b)throw std::runtime_error(m);}
J run(engine::Engine& e,const char* id,J p=J::object()){auto r=e.execute(id,p);check(r.ok,std::string(id)+": "+r.error);return r.value;}
void close(double a,double b){check(std::abs(a-b)<1e-4,"volume mismatch "+std::to_string(a)+" / "+std::to_string(b));}
int main(){try{
    for(bool round:{true,false}){
        engine::Engine e;run(e,"param.set",{{"name","R"},{"expression","3 mm"}});
        auto base=run(e,"primitive.add",{{"kind","box"},{"dimensions",{{"width","40 mm"},{"depth","40 mm"},{"height","20 mm"}}}});
        auto snapshot=regen::Regenerator{}.evaluate(e.session().document);auto entities=kernel::topology(*snapshot.bodies[0].exact);check(bool(entities),entities.error);J refs=J::array();
        for(const auto& r:*entities.value)if(r.signature.kind=="edge"&&std::abs(r.signature.direction[2])>.999)refs.push_back(doc::TopologyRef{base["body_id"],r.name,r.signature});
        check(refs.size()==4,"four vertical edges");J params={{"edges",refs},{round?"radius":"distance","R"}};
        auto f=run(e,round?"feature.fillet":"feature.chamfer",params);
        auto expected=[&](double w,double h,double r){return (w*40-(round?4-std::numbers::pi:2)*r*r)*h;};
        close(run(e,"model.mass_properties")["volume_mm3"],expected(40,20,3));
        run(e,"feature.edit",{{"feature_id",base["feature_id"]},{"dimensions",{{"width","60 mm"},{"height","35 mm"}}}});close(run(e,"model.mass_properties")["volume_mm3"],expected(60,35,3));
        auto count=kernel::topology(*regen::Regenerator{}.evaluate(e.session().document).bodies[0].exact);int faces=0;for(const auto& r:*count.value)if(r.signature.kind=="face")++faces;check(faces==10,"ten faces after four blends");
        run(e,"feature.edit",{{"feature_id",f["feature_id"]},{"params",{{round?"radius":"distance","2 mm"}}}});close(run(e,"model.mass_properties")["volume_mm3"],expected(60,35,2));run(e,"edit.undo");close(run(e,"model.mass_properties")["volume_mm3"],expected(60,35,3));
        auto saved=e.session().document;auto invalid=params;invalid[round?"radius":"distance"]="1000 mm";auto failure=e.execute(round?"feature.fillet":"feature.chamfer",invalid);check(!failure.ok&&!failure.error.empty()&&e.session().document==saved,"impossible feature atomic failure");
        run(e,"param.set",{{"name","R"},{"expression","1000 mm"}});auto error=run(e,"feature.get",{{"feature_id",f["feature_id"]}})["error"].get<std::string>();check(error.find("spigolo")!=std::string::npos&&error.find("primitive-1/")!=std::string::npos,"readable edge name in error: "+error);
        close(run(e,"model.mass_properties")["volume_mm3"],60*40*35);run(e,"param.set",{{"name","R"},{"expression","3 mm"}});close(run(e,"model.mass_properties")["volume_mm3"],expected(60,35,3));
    }
    std::cout<<"Fillet/chamfer: multiple stable edges, expressions, upstream changes, 10 faces, volume, edit/undo and readable non-crashing errors OK\n";
}catch(const std::exception& e){std::cerr<<e.what();return 1;}}
