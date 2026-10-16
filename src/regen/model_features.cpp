#include "regen/model_features.hpp"
#include "doc/expression.hpp"
#include "solver/sketch_solver.hpp"
#include "solver/profile_regions.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace sagomacad::regen {
namespace {
double value(const doc::Document& d,const nlohmann::json& j,const char* key){doc::Quantity q;std::string error;auto expression=j.at(key).get<std::string>();if(!doc::evaluate_expression(d,expression,q,error))throw std::runtime_error(std::string(key)+": "+error);if((q.dimension!=doc::Dimension::Length&&q.dimension!=doc::Dimension::Scalar)||!std::isfinite(q.value)||std::abs(q.value)>1e7)throw std::runtime_error(std::string(key)+": lunghezza non valida");return q.value;}
kernel::Shape checked(kernel::Result<kernel::Shape> r){if(!r)throw std::runtime_error(r.error);return *r.value;}
}
Regenerator::Cached build_model_feature(const doc::Document& d,const doc::Feature& f,const Snapshot& snapshot){
    try {
        const auto& data=f.data;auto id=data.at("sketch").get<std::string>();auto found=d.features.find(id);
        if(found==d.features.end()||found->second.type!="sketch")throw std::runtime_error("Schizzo sorgente mancante: "+id);
        if(found->second.suppressed)throw std::runtime_error("Schizzo sorgente soppresso: "+id);
        auto before=std::find_if(snapshot.features.begin(),snapshot.features.end(),[&](const auto& row){return row.id==id;});
        if(before==snapshot.features.end())throw std::runtime_error("Lo schizzo deve precedere la feature: "+id);
        if(!before->error.empty())throw std::runtime_error("Schizzo sorgente in errore: "+before->error);
        auto solved=solver::solve(d,doc::read_sketch(found->second.data));if(!solved.error.empty())throw std::runtime_error(solved.error);
        const auto regions=solver::profile_regions(solved.sketch,solved.profiles);
        const auto& selected=data.at("profiles");if(!selected.is_array()||selected.empty()||selected.size()>1024)throw std::runtime_error("Seleziona almeno un profilo");
        std::optional<kernel::Shape> shape;std::vector<std::string> used;
        for(const auto& ref:selected){
            auto region=std::find_if(regions.begin(),regions.end(),[&](const auto& r){if(ref.is_string())return r.id==ref.get<std::string>();if(ref.is_array()){auto ids=ref.get<std::vector<std::string>>(),boundary=r.boundary;std::sort(ids.begin(),ids.end());std::sort(boundary.begin(),boundary.end());return ids==boundary;}return false;});
            if(region==regions.end())throw std::runtime_error("Profilo non più disponibile: "+ref.dump());
            if(std::find(used.begin(),used.end(),region->id)!=used.end())throw std::runtime_error("Profilo duplicato");used.push_back(region->id);
            auto face=checked(kernel::profile_face(solved.sketch,region->boundary,region->holes));
            double distance=value(d,data,"distance"),start=0;
            auto direction=data.value("direction",std::string("one_side"));auto extent=data.value("extent",std::string("distance"));
            if(direction!="one_side"&&direction!="symmetric"&&direction!="two_sides")throw std::runtime_error("Direzione non valida");
            if(extent!="distance"&&extent!="through_all")throw std::runtime_error("Estensione non valida");
            const kernel::Vec3 normal=solved.sketch.plane=="XY"?kernel::Vec3{0,0,1}:solved.sketch.plane=="XZ"?kernel::Vec3{0,-1,0}:kernel::Vec3{1,0,0};
            if(extent=="through_all"){
                if(data.value("operation",std::string("new"))=="new")throw std::runtime_error("Attraverso tutto richiede corpi di destinazione");
                double low=0,high=0;bool target=false;
                for(const auto& body:snapshot.bodies)if(body.exact&&std::find(data.at("target_bodies").begin(),data.at("target_bodies").end(),body.id)!=data.at("target_bodies").end()){
                    auto mass=kernel::mass_properties(*body.exact);if(!mass)throw std::runtime_error(mass.error);target=true;
                    auto projection=[&](kernel::Vec3 p){return p.x*normal.x+p.y*normal.y+p.z*normal.z;};
                    low=std::min({low,projection(mass.value->minimum),projection(mass.value->maximum)});high=std::max({high,projection(mass.value->minimum),projection(mass.value->maximum)});
                }
                if(!target)throw std::runtime_error("Nessun corpo di destinazione disponibile");
                const double margin=std::max(1.,(high-low)*.01);
                if(direction=="one_side"){if(distance<0){start=low-margin;distance=-start;}else distance=high+margin;}
                else {start=low-margin;distance=high-low+2*margin;}
            }else if(direction=="symmetric")start=-distance/2;
            else if(direction=="two_sides"){double second=value(d,data,"second_distance");if(distance<=0||second<0)throw std::runtime_error("Le distanze dei due lati devono essere positive");start=-second;distance+=second;}
            if(std::abs(distance)<1e-7)throw std::runtime_error("La distanza deve essere diversa da zero");
            face=checked(kernel::translate(face,normal.x*start,normal.y*start,normal.z*start));
            auto swept=checked(kernel::extrude(face,{normal.x*distance,normal.y*distance,normal.z*distance}));
            shape=shape?checked(kernel::boolean_operation(*shape,swept,"join")):swept;
        }
        return {shape,{}};
    }catch(const std::exception& e){return {{},e.what()};}catch(...){return {{},"Valutazione della feature fallita"};}
}
}
