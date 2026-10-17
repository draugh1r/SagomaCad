#include "regen/model_features.hpp"
#include "kernel/topology.hpp"
#include "regen/attached_sketch.hpp"
#include "regen/extrusion.hpp"
#include "doc/expression.hpp"
#include "solver/sketch_solver.hpp"
#include "solver/profile_regions.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <numbers>
namespace sagomacad::regen {
namespace {
double value(const doc::Document& d,const nlohmann::json& j,const char* key){doc::Quantity q;std::string error;auto expression=j.at(key).get<std::string>();if(!doc::evaluate_expression(d,expression,q,error))throw std::runtime_error(std::string(key)+": "+error);if((q.dimension!=doc::Dimension::Length&&q.dimension!=doc::Dimension::Scalar)||!std::isfinite(q.value)||std::abs(q.value)>1e7)throw std::runtime_error(std::string(key)+": lunghezza non valida");return q.value;}
kernel::Shape checked(kernel::Result<kernel::Shape> r){if(!r)throw std::runtime_error(r.error);return *r.value;}
}
Regenerator::Cached build_model_feature(const doc::Document& d,const doc::Feature& f,const Snapshot& snapshot){
    try {
        const auto& data=f.data;
        if(data.contains("face")){
            if(f.type!="feature.extrude")throw std::runtime_error("La faccia è ammessa solo per Estrudi");
            auto ref=data.at("face").get<doc::TopologyRef>();auto body=std::find_if(snapshot.bodies.begin(),snapshot.bodies.end(),[&](const auto& b){return b.id==ref.body;});
            if(body==snapshot.bodies.end()||!body->exact)throw std::runtime_error("Corpo della faccia sorgente assente: "+ref.body);
            auto face=kernel::resolve(*body->exact,ref,"face");if(!face)throw std::runtime_error(face.error);
            auto frame=kernel::face_frame(*body->exact,face.value->id);if(!frame)throw std::runtime_error(frame.error);
            auto swept=sweep_profile(d,data,snapshot,checked(kernel::face_shape(*body->exact,face.value->id)),frame.value->normal,frame.value->origin);
            if(!swept)throw std::runtime_error(swept.error);auto named=kernel::name_topology(*swept.value,f.id);return {named.value,named.error};
        }
        auto id=data.at("sketch").get<std::string>();auto found=d.features.find(id);
        if(found==d.features.end()||found->second.type!="sketch")throw std::runtime_error("Schizzo sorgente mancante: "+id);
        if(found->second.suppressed)throw std::runtime_error("Schizzo sorgente soppresso: "+id);
        auto before=std::find_if(snapshot.features.begin(),snapshot.features.end(),[&](const auto& row){return row.id==id;});
        if(before==snapshot.features.end())throw std::runtime_error("Lo schizzo deve precedere la feature: "+id);
        if(!before->error.empty())throw std::runtime_error("Schizzo sorgente in errore: "+before->error);
        auto solved=solver::solve(d,evaluated_sketch(d,id));if(!solved.error.empty())throw std::runtime_error(solved.error);
        const auto regions=solver::profile_regions(solved.sketch,solved.profiles);
        const auto& selected=data.at("profiles");if(!selected.is_array()||selected.empty()||selected.size()>1024)throw std::runtime_error("Seleziona almeno un profilo");
        std::optional<kernel::Shape> shape;std::vector<std::string> used;
        for(const auto& ref:selected){
            auto region=std::find_if(regions.begin(),regions.end(),[&](const auto& r){if(ref.is_string())return r.id==ref.get<std::string>();if(ref.is_array()){auto ids=ref.get<std::vector<std::string>>(),boundary=r.boundary;std::sort(ids.begin(),ids.end());std::sort(boundary.begin(),boundary.end());return ids==boundary;}return false;});
            if(region==regions.end())throw std::runtime_error("Profilo non più disponibile: "+ref.dump());
            if(std::find(used.begin(),used.end(),region->id)!=used.end())throw std::runtime_error("Profilo duplicato");used.push_back(region->id);
            auto face=checked(kernel::profile_face(solved.sketch,region->boundary,region->holes));
            if(f.type=="feature.revolve"){
                auto axis=data.at("axis").get<std::string>();kernel::Vec3 origin{},direction{};
                if(axis=="X")direction={1,0,0};else if(axis=="Y")direction={0,1,0};else if(axis=="Z")direction={0,0,1};
                else {
                    auto line=std::find_if(solved.sketch.entities.begin(),solved.sketch.entities.end(),[&](const auto& e){return e.id==axis&&e.kind=="line";});
                    if(line==solved.sketch.entities.end())throw std::runtime_error("Asse non disponibile: "+axis);
                    auto point=[&](const std::string& pid){for(const auto& p:solved.sketch.points)if(p.id==pid)return solved.sketch.plane=="XY"?kernel::Vec3{p.x,p.y,0}:solved.sketch.plane=="XZ"?kernel::Vec3{p.x,0,p.y}:kernel::Vec3{0,p.x,p.y};throw std::runtime_error("Punto dell'asse mancante");};
                    origin=point(line->points[0]);auto end=point(line->points[1]);direction={end.x-origin.x,end.y-origin.y,end.z-origin.z};
                }
                auto type=data.value("type",std::string("angle"));double angle=2*std::numbers::pi;
                if(type=="angle"){
                    doc::Quantity q;std::string error;if(!doc::evaluate_expression(d,data.at("angle").get<std::string>(),q,error))throw std::runtime_error("Angolo: "+error);
                    if(q.dimension!=doc::Dimension::Scalar&&q.dimension!=doc::Dimension::Angle)throw std::runtime_error("È richiesto un angolo");
                    angle=q.dimension==doc::Dimension::Scalar?q.value*std::numbers::pi/180:q.value;
                }else if(type!="full")throw std::runtime_error("Tipo rivoluzione non valido: angle o full");
                auto swept=checked(kernel::revolve(face,origin,direction,angle));shape=shape?checked(kernel::boolean_operation(*shape,swept,"join")):swept;continue;
            }
            const auto& sk=solved.sketch;
            const kernel::Vec3 normal=sk.support?kernel::Vec3{sk.u[1]*sk.v[2]-sk.u[2]*sk.v[1],sk.u[2]*sk.v[0]-sk.u[0]*sk.v[2],sk.u[0]*sk.v[1]-sk.u[1]*sk.v[0]}:sk.plane=="XY"?kernel::Vec3{0,0,1}:sk.plane=="XZ"?kernel::Vec3{0,-1,0}:kernel::Vec3{1,0,0};
            auto swept=checked(sweep_profile(d,data,snapshot,face,normal,{sk.origin[0],sk.origin[1],sk.origin[2]}));
            shape=shape?checked(kernel::boolean_operation(*shape,swept,"join")):swept;
        }
        auto named=kernel::name_topology(*shape,f.id);return {named.value,named.error};
    }catch(const std::exception& e){return {{},e.what()};}catch(...){return {{},"Valutazione della feature fallita"};}
}
}
