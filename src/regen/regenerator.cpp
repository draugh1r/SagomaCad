#include "regen/regenerator.hpp"
#include "doc/expression.hpp"
#include "solver/sketch_solver.hpp"
#include "regen/model_features.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace sagomacad::regen {
namespace {
double length(const doc::Document& document,const nlohmann::json& object,const char* key,bool positive) {
    if (!object.is_object() || !object.contains(key) || !object[key].is_string())
        throw std::runtime_error(std::string("Manca l'espressione ")+key);
    doc::Quantity quantity;
    std::string error;
    if (!doc::evaluate_expression(document,object[key].get<std::string>(),quantity,error))
        throw std::runtime_error(std::string(key)+": "+error);
    if (quantity.dimension!=doc::Dimension::Length && quantity.dimension!=doc::Dimension::Scalar)
        throw std::runtime_error(std::string(key)+": è richiesta una lunghezza");
    if (!std::isfinite(quantity.value) || std::abs(quantity.value)>1.0e9 || (positive && quantity.value<=0.0))
        throw std::runtime_error(std::string(key)+": valore fuori intervallo");
    return quantity.value;
}
Regenerator::Cached build(const doc::Document& document,const doc::Feature& feature) {
    try {
        if (feature.kind!=doc::FeatureKind::Exact) return {{},"Feature SDF non disponibile in M2"};
        if (!feature.data.is_object()) throw std::runtime_error("Dati della feature non validi");
        const auto& dimensions=feature.data.at("dimensions");
        const auto position=feature.data.value("position",nlohmann::json{{"x","0 mm"},{"y","0 mm"},{"z","0 mm"}});
        const double x=length(document,position,"x",false);
        const double y=length(document,position,"y",false);
        const double z=length(document,position,"z",false);
        kernel::Result<kernel::Shape> shape;
        if (feature.type=="primitive.box")
            shape=kernel::make_box(length(document,dimensions,"width",true),
                                   length(document,dimensions,"depth",true),
                                   length(document,dimensions,"height",true));
        else if (feature.type=="primitive.cylinder")
            shape=kernel::make_cylinder(length(document,dimensions,"radius",true),
                                        length(document,dimensions,"height",true));
        else if (feature.type=="primitive.sphere")
            shape=kernel::make_sphere(length(document,dimensions,"radius",true));
        else return {{},"Tipo di feature non supportato: "+feature.type};
        if (!shape) return {{},shape.error};
        auto moved=kernel::translate(*shape.value,x,y,z);
        if (!moved) return {{},moved.error};
        return {std::move(moved.value),{}};
    } catch (const std::exception& e) { return {{},e.what()}; }
      catch (...) { return {{},"Errore imprevisto nella rigenerazione"}; }
}
}
const Snapshot& Regenerator::evaluate(const doc::Document& document) {
    const std::string revision=doc::to_json(document).dump();
    if (revision_!=revision) { revision_=revision; cache_.clear(); }
    snapshot_={};
    for (const auto& body:document.bodies) snapshot_.bodies.push_back({body.id,{},{},{}});
    const auto count=std::min(document.rollback,document.timeline.size());
    for (std::size_t i=0;i<count;++i) {
        const auto& id=document.timeline[i];
        const auto it=document.features.find(id);
        if (it==document.features.end()) {
            snapshot_.features.push_back({id,"Feature mancante nella timeline",false});
            continue;
        }
        if (it->second.suppressed) { snapshot_.features.push_back({id,{},false}); continue; }
        if (it->second.type=="sketch") {
            try {
                auto result=solver::solve(document,doc::read_sketch(it->second.data));
                snapshot_.features.push_back({id,result.error,false});
            } catch(const std::exception& e) { snapshot_.features.push_back({id,e.what(),false}); }
            continue;
        }
        auto [cached,inserted]=cache_.try_emplace(id);
        if (inserted) cached->second=it->second.type=="feature.extrude"?build_model_feature(document,it->second,snapshot_):build(document,it->second);
        else ++snapshot_.cache_hits;
        const auto& result=cached->second;
        snapshot_.features.push_back({id,result.error,!inserted});
        try {
            const auto body_id=it->second.data.at("body_id").get<std::string>();
            const auto body=std::find_if(snapshot_.bodies.begin(),snapshot_.bodies.end(),
                [&](const BodyState& row){return row.id==body_id;});
            if (body==snapshot_.bodies.end()) {
                snapshot_.features.back().error="Corpo della feature non trovato";
                continue;
            }
            const auto operation=it->second.data.value("operation",std::string("new"));
            if(it->second.type=="feature.extrude" && operation!="new") {
                if(!result.shape) {snapshot_.features.back().error=result.error;continue;}
                std::vector<std::pair<BodyState*,kernel::Shape>> pending;
                for(const auto& target:it->second.data.at("target_bodies")) {
                    auto dest=std::find_if(snapshot_.bodies.begin(),snapshot_.bodies.end(),[&](const auto& b){return b.id==target.get<std::string>();});
                    if(dest==snapshot_.bodies.end()||!dest->exact)throw std::runtime_error("Corpo di destinazione assente: "+target.get<std::string>());
                    auto combined=kernel::boolean_operation(*dest->exact,*result.shape,operation);
                    if(!combined)throw std::runtime_error(combined.error);
                    pending.emplace_back(&*dest,*combined.value);
                }
                if(pending.empty())throw std::runtime_error("Corpi di destinazione mancanti");
                for(auto& [dest,shape]:pending){dest->exact=shape;dest->final=shape;dest->error.clear();}
                continue;
            }
            if (operation=="subtract") {
                if (!body->exact || !result.shape) {
                    snapshot_.features.back().error=result.error.empty()?"Corpo sorgente o utensile del taglio assente":result.error;
                    continue;
                }
                auto cut=kernel::subtract(*body->exact,*result.shape);
                if (!cut) {
                    snapshot_.features.back().error=cut.error;
                    body->error=cut.error;
                    continue;
                }
                body->exact=*cut.value;
                body->final=*cut.value;
                body->error.clear();
            } else if (operation=="new") {
                body->exact=result.shape;
                body->final=result.shape;
                body->error=result.error;
            } else snapshot_.features.back().error="Operazione sconosciuta: "+operation;
        } catch (const std::exception& e) { snapshot_.features.back().error=e.what(); }
    }
    return snapshot_;
}
} // namespace sagomacad::regen
