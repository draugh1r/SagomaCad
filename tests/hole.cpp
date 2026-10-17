#include "engine/command.hpp"
#include "regen/regenerator.hpp"
#include "kernel/topology.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>
using namespace sagomacad;using J=nlohmann::json;
void check(bool b,const std::string& m){if(!b)throw std::runtime_error(m);}
J run(engine::Engine& e,const char* id,J p=J::object()){auto r=e.execute(id,p);check(r.ok,std::string(id)+": "+r.error);return r.value;}
void close(double a,double b){check(std::abs(a-b)<1e-4,"mismatch "+std::to_string(a)+" / "+std::to_string(b));}
int main(){try{
    for(bool through:{true,false}){
        engine::Engine e;run(e,"param.set",{{"name","offset"},{"expression","10 mm"}});
        auto base=run(e,"primitive.add",{{"kind","box"},{"dimensions",{{"width","40 mm"},{"depth","40 mm"},{"height","20 mm"}}}});J ref={{"body",base["body_id"]},{"name",base["feature_id"].get<std::string>()+"/z_max"}};
        auto hole=run(e,"feature.hole",{{"face",ref},{"position",{{"x",10},{"y",10}}},{"diameter","8 mm"},{"depth","6 mm"},{"through_all",through}});
        close(run(e,"model.mass_properties")["volume_mm3"],32000-16*std::numbers::pi*(through?20:6));
        auto feature=run(e,"feature.get",{{"feature_id",hole["feature_id"]}});check(feature["params"]["references"].size()==2,"two position dimensions");
        run(e,"feature.edit",{{"feature_id",base["feature_id"]},{"dimensions",{{"width","60 mm"},{"height","35 mm"}}}});close(run(e,"model.mass_properties")["volume_mm3"],84000-16*std::numbers::pi*(through?35:6));
        auto entities=kernel::topology(*regen::Regenerator{}.evaluate(e.session().document).bodies[0].exact);int faces=0;
        for(const auto& entity:*entities.value){if(entity.signature.kind=="face")++faces;if(entity.signature.type=="cylinder"){close(entity.signature.center[0],10);close(entity.signature.center[1],10);}}
        check(faces==(through?7:8),"hole face count");
        auto references=feature["params"]["references"];references[0]["distance"]="offset";run(e,"feature.edit",{{"feature_id",hole["feature_id"]},{"params",{{"references",references}}}});run(e,"param.set",{{"name","offset"},{"expression","12 mm"}});
        entities=kernel::topology(*regen::Regenerator{}.evaluate(e.session().document).bodies[0].exact);for(const auto& entity:*entities.value)if(entity.signature.type=="cylinder"){std::array<double,2> xy={entity.signature.center[0],entity.signature.center[1]};std::sort(xy.begin(),xy.end());close(xy[0],10);close(xy[1],12);}
        auto saved=e.session().document;auto bad=e.execute("feature.hole",{{"face",ref},{"position",{{"x",-100},{"y",-100}}},{"diameter","8 mm"},{"through_all",true}});check(!bad.ok&&e.session().document==saved,"outside position atomic error");
        doc::Document copy;std::string error;check(doc::from_json(doc::to_json(saved),copy,error),error);e.session().document=copy;close(run(e,"model.mass_properties")["volume_mm3"],84000-16*std::numbers::pi*(through?35:6));
    }
    engine::Engine gone;auto base=run(gone,"primitive.add",{{"kind","box"},{"dimensions",{{"width","40 mm"},{"depth","40 mm"},{"height","20 mm"}}}});auto cut=run(gone,"primitive.add",{{"kind","box"},{"operation","subtract"},{"body",base["body_id"]},{"position",{{"z","10 mm"}}},{"dimensions",{{"width","5 mm"},{"depth","40 mm"},{"height","10 mm"}}}});
    auto hole=run(gone,"feature.hole",{{"face",{{"body",base["body_id"]},{"name",base["feature_id"].get<std::string>()+"/z_max"}}},{"position",{{"x",15},{"y",15}}},{"diameter","8 mm"},{"through_all",true}});run(gone,"feature.edit",{{"feature_id",cut["feature_id"]},{"dimensions",{{"width","40 mm"}}}});
    check(run(gone,"feature.get",{{"feature_id",hole["feature_id"]}})["error"].get<std::string>().find("eliminata")!=std::string::npos,"deleted support readable hole error");
    std::cout<<"Hole: diameter 8, through/blind volume and face counts, two edge distances, upstream edits, expression, persistence and deleted support OK\n";
}catch(const std::exception& e){std::cerr<<e.what();return 1;}}
