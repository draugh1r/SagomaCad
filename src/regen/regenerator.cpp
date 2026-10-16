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
bool sketch_visible(const doc::Document& document,const Snapshot& snapshot,const std::string& id,const std::string& editing) {
    const auto f=document.features.find(id);
    if(f==document.features.end()||f->second.type!="sketch"||f->second.suppressed)return false;
    if(std::none_of(snapshot.features.begin(),snapshot.features.end(),[&](const auto& row){return row.id==id;}))return false;
    if(id==editing)return true;
    const auto& data=f->second.data;
    if(data.contains("visible")&&data["visible"].is_boolean())return data["visible"].get<bool>();
    return std::find(snapshot.hidden_sketches.begin(),snapshot.hidden_sketches.end(),id)==snapshot.hidden_sketches.end();
}
const Snapshot& Regenerator::evaluate(const doc::Document& document) {
    snapshot_={};
    for (const auto& body:document.bodies) snapshot_.bodies.push_back({body.id,{},{},{}});
    const auto count=std::min(document.rollback,document.timeline.size());
    bool dirty=false;
    for (std::size_t i=0;i<count;++i) {
        const auto& id=document.timeline[i];
        const auto it=document.features.find(id);
        nlohmann::json signature={{"id",id}};
        std::vector<std::string> affected;
        if(it!=document.features.end()) {
            const auto& f=it->second;
            signature["feature"]={{"type",f.type},{"kind",int(f.kind)},{"suppressed",f.suppressed},{"data",f.data}};
            // Only evaluated expressions influence the cache: unrelated parameters and
            // display metadata must not force geometric work on earlier features.
            auto evaluate=[&](const nlohmann::json& expression){if(!expression.is_string())return;doc::Quantity q;std::string error;
                if(doc::evaluate_expression(document,expression.get<std::string>(),q,error))signature["values"].push_back({q.value,int(q.dimension)});else signature["values"].push_back(error);};
            if(f.data.is_object()) {
                for(const char* key:{"distance","second_distance","angle"})if(f.data.contains(key))evaluate(f.data[key]);
                for(const char* key:{"dimensions","position","constraints"})if(f.data.contains(key)) {
                    const auto& rows=f.data[key];if(rows.is_array()){for(const auto& row:rows)if(row.is_object()&&row.contains("expression"))evaluate(row["expression"]);}
                    else if(rows.is_object())for(const auto& expression:rows)evaluate(expression);
                }
                if(f.data.contains("body_id")&&f.data["body_id"].is_string())affected.push_back(f.data["body_id"].get<std::string>());
                if(f.data.contains("target_bodies")&&f.data["target_bodies"].is_array())for(const auto& target:f.data["target_bodies"])if(target.is_string())affected.push_back(target.get<std::string>());
                for(const auto& target:affected)signature["targets"].push_back({target,std::any_of(document.bodies.begin(),document.bodies.end(),[&](const auto& b){return b.id==target&&b.kind==doc::BodyKind::BRep;})});
            }
        }
        const auto key=signature.dump();
        if(!dirty&&i<steps_.size()&&steps_[i].key==key) {
            auto state=steps_[i].feature;state.cached=true;snapshot_.features.push_back(state);++snapshot_.cache_hits;
            for(const auto& saved:steps_[i].changed_bodies)for(auto& body:snapshot_.bodies)if(body.id==saved.id)body=saved;
            continue;
        }
        if(!dirty){steps_.resize(i);dirty=true;}
        auto process=[&] {
        if (it==document.features.end()) {
            snapshot_.features.push_back({id,"Feature mancante nella timeline",false});
            return;
        }
        if (it->second.suppressed) { snapshot_.features.push_back({id,{},false}); return; }
        if (it->second.type=="sketch") {
            try {
                auto result=solver::solve(document,doc::read_sketch(it->second.data));
                snapshot_.features.push_back({id,result.error,false});
            } catch(const std::exception& e) { snapshot_.features.push_back({id,e.what(),false}); }
            return;
        }
        const auto result=it->second.type=="feature.extrude"||it->second.type=="feature.revolve"?build_model_feature(document,it->second,snapshot_):build(document,it->second);
        snapshot_.features.push_back({id,result.error,false});
        try {
            const auto body_id=it->second.data.at("body_id").get<std::string>();
            const auto body=std::find_if(snapshot_.bodies.begin(),snapshot_.bodies.end(),
                [&](const BodyState& row){return row.id==body_id;});
            if (body==snapshot_.bodies.end()) {
                snapshot_.features.back().error="Corpo della feature non trovato";
                return;
            }
            const auto operation=it->second.data.value("operation",std::string("new"));
            if((it->second.type=="feature.extrude"||it->second.type=="feature.revolve") && operation!="new") {
                if(!result.shape) {snapshot_.features.back().error=result.error;return;}
                std::vector<std::pair<BodyState*,kernel::Shape>> pending;
                std::optional<kernel::Shape> joined=operation=="join"?result.shape:std::nullopt;
                for(const auto& target:it->second.data.at("target_bodies")) {
                    auto dest=std::find_if(snapshot_.bodies.begin(),snapshot_.bodies.end(),[&](const auto& b){return b.id==target.get<std::string>();});
                    if(dest==snapshot_.bodies.end()||!dest->exact)throw std::runtime_error("Corpo di destinazione assente: "+target.get<std::string>());
                    auto combined=kernel::boolean_operation(*dest->exact,joined?*joined:*result.shape,operation);
                    if(!combined)throw std::runtime_error(combined.error);
                    if(joined)joined=*combined.value;
                    pending.emplace_back(&*dest,*combined.value);
                }
                if(pending.empty())throw std::runtime_error("Corpi di destinazione mancanti");
                for(size_t index=0;index<pending.size();++index){auto& [dest,shape]=pending[index];
                    if(joined&&index>0){dest->exact.reset();dest->final.reset();}
                    else {dest->exact=joined?*joined:shape;dest->final=dest->exact;}
                    dest->error.clear();
                }
                return;
            }
            if (operation=="subtract") {
                if (!body->exact || !result.shape) {
                    snapshot_.features.back().error=result.error.empty()?"Corpo sorgente o utensile del taglio assente":result.error;
                    return;
                }
                auto cut=kernel::subtract(*body->exact,*result.shape);
                if (!cut) {
                    snapshot_.features.back().error=cut.error;
                    body->error=cut.error;
                    return;
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
        };
        process();
        Step step{key,snapshot_.features.back(),{}};
        std::sort(affected.begin(),affected.end());affected.erase(std::unique(affected.begin(),affected.end()),affected.end());
        for(const auto& body:snapshot_.bodies)if(std::find(affected.begin(),affected.end(),body.id)!=affected.end())step.changed_bodies.push_back(body);
        steps_.push_back(std::move(step));
    }
    if(count==document.timeline.size())steps_.resize(count);
    for(const auto& state:snapshot_.features)if(state.error.empty()) {
        auto f=document.features.find(state.id);
        if(f!=document.features.end()&&!f->second.suppressed&&(f->second.type=="feature.extrude"||f->second.type=="feature.revolve")&&f->second.data.contains("sketch")&&f->second.data["sketch"].is_string())snapshot_.hidden_sketches.push_back(f->second.data["sketch"].get<std::string>());
    }
    return snapshot_;
}
} // namespace sagomacad::regen
