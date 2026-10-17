#include "regen/attached_sketch.hpp"
#include "kernel/topology.hpp"
#include <algorithm>
#include <set>
#include <stdexcept>
namespace sagomacad::regen {
doc::Sketch attached_sketch(doc::Sketch sketch,const Snapshot& snapshot){
    if(!sketch.support)return sketch;
    const auto& ref=*sketch.support;auto body=std::find_if(snapshot.bodies.begin(),snapshot.bodies.end(),[&](const auto& b){return b.id==ref.body;});
    if(body==snapshot.bodies.end()||!body->exact)throw std::runtime_error("Corpo della faccia di supporto assente: "+ref.body);
    auto face=kernel::resolve(*body->exact,ref,"face");if(!face)throw std::runtime_error(face.error);
    sketch.support->signature=face.value->signature;
    auto projected=kernel::face_reference_geometry(*body->exact,face.value->id);if(!projected)throw std::runtime_error(projected.error);
    std::set<std::string> old_points;for(const auto& e:sketch.entities)if(!e.reference_name.empty())old_points.insert(e.points.begin(),e.points.end());
    std::erase_if(sketch.entities,[](const auto& e){return !e.reference_name.empty();});std::erase_if(sketch.points,[&](const auto& p){return old_points.contains(p.id);});
    sketch.entities.insert(sketch.entities.end(),projected.value->entities.begin(),projected.value->entities.end());sketch.points.insert(sketch.points.end(),projected.value->points.begin(),projected.value->points.end());
    sketch.origin=projected.value->origin;sketch.u=projected.value->u;sketch.v=projected.value->v;
    return doc::read_sketch(doc::sketch_json(sketch));
}
doc::Sketch evaluated_sketch(const doc::Document& document,const std::string& id){
    auto found=document.features.find(id);if(found==document.features.end()||found->second.type!="sketch"||found->second.suppressed)throw std::runtime_error("Schizzo sorgente mancante o soppresso: "+id);
    auto sketch=doc::read_sketch(found->second.data);if(!sketch.support)return sketch;
    auto prefix=document;auto at=std::find(prefix.timeline.begin(),prefix.timeline.end(),id);if(at==prefix.timeline.end())throw std::runtime_error("Schizzo fuori timeline: "+id);
    prefix.rollback=std::size_t(at-prefix.timeline.begin());Regenerator r;return attached_sketch(std::move(sketch),r.evaluate(prefix));
}
}
