#include "engine/command.hpp"
#include "engine/cmds/validation.hpp"
#include "kernel/measure.hpp"
#include "regen/regenerator.hpp"
#include <algorithm>
#include <cmath>
#include <set>
namespace sagomacad::engine::cmds {
namespace {
using J=nlohmann::json;
struct Resolved {kernel::Shape shape;kernel::TopologyEntity entity;doc::TopologyRef ref;};
Resolved resolve_ref(const regen::Snapshot& snapshot,const J& input){
    auto ref=input.get<doc::TopologyRef>();auto kind=input.value("kind",ref.signature.kind);if(kind!="face"&&kind!="edge"&&kind!="vertex")throw std::runtime_error("Specifica kind o una firma geometrica di faccia, spigolo o vertice");
    auto body=std::find_if(snapshot.bodies.begin(),snapshot.bodies.end(),[&](const auto& b){return b.id==ref.body;});if(body==snapshot.bodies.end()||!body->exact)throw std::runtime_error("Corpo del riferimento assente: "+ref.body);
    auto entity=kernel::resolve(*body->exact,ref,kind);if(!entity)throw std::runtime_error(entity.error);ref.signature=entity.value->signature;return {*body->exact,*entity.value,ref};
}
std::array<double,3> vector(const J& p,const char* key){auto j=p.at(key);if(!j.is_array()||j.size()!=3)throw std::runtime_error(std::string(key)+": vettore di tre numeri richiesto");auto v=j.get<std::array<double,3>>();double length=std::hypot(v[0],v[1],v[2]);if(!std::isfinite(length)||length<1e-9)throw std::runtime_error("Direzione non valida");for(auto& n:v)n/=length;return v;}
CommandResult query(Session& s,const J& p){
    only_keys(p,{"body","kind","type","parallel_to","normal","limit"});std::string kind=p.contains("kind")?string_arg(p,"kind",true,16):"all",type=p.contains("type")?string_arg(p,"type",true,64):"",body=p.contains("body")?string_arg(p,"body",true,256):"";
    if(kind!="all"&&kind!="face"&&kind!="edge"&&kind!="vertex")throw std::runtime_error("kind deve essere face, edge, vertex o all");int limit=p.value("limit",4096);if(limit<1||limit>16384)throw std::runtime_error("limit fuori intervallo 1–16384");
    std::optional<std::array<double,3>> parallel,normal;if(p.contains("parallel_to"))parallel=vector(p,"parallel_to");if(p.contains("normal"))normal=vector(p,"normal");
    regen::Regenerator r;auto snapshot=r.evaluate(s.document);J output=J::array();bool exists=body.empty();bool truncated=false;
    for(const auto& b:snapshot.bodies){if(!body.empty()&&b.id!=body)continue;exists=true;if(!b.exact)continue;auto entities=kernel::topology(*b.exact);if(!entities)throw std::runtime_error(entities.error);
        for(const auto& e:*entities.value){const auto& g=e.signature;if(kind!="all"&&g.kind!=kind)continue;if(!type.empty()&&g.type!=type)continue;
            double n=std::hypot(g.direction[0],g.direction[1],g.direction[2]);auto dot=[&](auto v){return n>1e-9?(v[0]*g.direction[0]+v[1]*g.direction[1]+v[2]*g.direction[2])/n:0.;};
            if(parallel&&std::abs(dot(*parallel))<1-1e-7)continue;if(normal&&(g.kind!="face"||dot(*normal)<1-1e-7))continue;
            if(output.size()>=std::size_t(limit)){truncated=true;continue;}
            J boundary=J::array();for(int id:e.edges)for(const auto& edge:*entities.value)if(edge.signature.kind=="edge"&&edge.id==id)boundary.push_back(doc::TopologyRef{b.id,edge.name,edge.signature});
            bool ambiguous=std::count_if(entities.value->begin(),entities.value->end(),[&](const auto& row){return row.name==e.name;})>1;
            const auto description=(g.kind=="face"?"Faccia ":g.kind=="edge"?"Spigolo ":"Vertice ")+g.type+", "+std::to_string(g.measure)+(g.kind=="face"?" mm²":g.kind=="edge"?" mm":"");
            output.push_back({{"ref",doc::TopologyRef{b.id,e.name,g}},{"id",e.id},{"kind",g.kind},{"name",e.name},{"geometry",g},{"description",description},{"boundary",boundary},{"ambiguous",ambiguous}});
        }
    }if(!exists)throw std::runtime_error("Corpo sconosciuto: "+body);return {true,{{"entities",output},{"truncated",truncated}}, {}};
}
J canonical_selection(const doc::Document& d,const J& inputs,bool skip_missing){
    if(!inputs.is_array()||inputs.size()>4096)throw std::runtime_error("refs deve essere una lista di massimo 4096 elementi");regen::Regenerator r;auto snapshot=r.evaluate(d);J output=J::array();std::set<std::string> seen;
    for(const auto& item:inputs){try{
        if(!item.is_object())throw std::runtime_error("Riferimento di selezione non valido");auto kind=item.value("kind",item.value("signature",J::object()).value("kind",std::string()));J entry;
        if(kind=="body"){auto id=string_arg(item,"body",true,256);auto body=std::find_if(snapshot.bodies.begin(),snapshot.bodies.end(),[&](const auto& b){return b.id==id&&bool(b.exact);});if(body==snapshot.bodies.end())throw std::runtime_error("Corpo selezionato assente: "+id);entry={{"kind",kind},{"body",id},{"name",id},{"id",0}};}
        else if(kind=="sketch"){auto id=string_arg(item,"name",true,256);auto f=d.features.find(id);if(f==d.features.end()||f->second.type!="sketch")throw std::runtime_error("Schizzo selezionato assente");entry={{"kind",kind},{"name",id},{"body",""},{"id",0}};}
        else{auto ref=resolve_ref(snapshot,item);entry=ref.ref;entry["kind"]=kind;entry["id"]=ref.entity.id;}
        auto key=kind+entry.value("body",std::string())+entry.at("name").get<std::string>();if(seen.insert(key).second)output.push_back(std::move(entry));
    }catch(...){if(!skip_missing)throw;}}
    return output;
}
}
std::vector<CommandSpec> geometry_specs(){return {
    {"geometry.query","Interroga geometria",{"Ispeziona"},std::nullopt,"{body?:id, kind?:all|face|edge|vertex, type?:plane|line|circle|cylinder|..., parallel_to?:[x,y,z], normal?:[x,y,z], limit?:1..16384}; riferimenti stabili e descrizioni geometriche",{},query,false},
    {"inspect.measure","Misura",{"Ispeziona"},std::nullopt,"{a:{body,name,signature?,kind?}, b:{body,name,signature?,kind?}}; distanza minima mm, angolo acuto in gradi (null se non definito)",{},
        [](Session& s,const J& p)->CommandResult{only_keys(p,{"a","b"});regen::Regenerator r;auto snapshot=r.evaluate(s.document);auto a=resolve_ref(snapshot,p.at("a")),b=resolve_ref(snapshot,p.at("b"));auto value=kernel::measure(a.shape,a.entity,b.shape,b.entity);if(!value)throw std::runtime_error(value.error);return {true,{{"distance_mm",value.value->distance},{"angle_degrees",value.value->angle_degrees?J(*value.value->angle_degrees):J()},{"a",a.ref},{"b",b.ref}}, {}};},false},
    {"selection.set","Seleziona entità",{"Seleziona"},std::nullopt,"{refs:[{kind,body?,name,signature?},...]}; stato della selezione nella sessione",{},
        [](Session& s,const J& p)->CommandResult{only_keys(p,{"refs"});auto refs=canonical_selection(s.document,p.at("refs"),false);s.selection=refs;return {true,{{"refs",refs}}, {}};},false},
    {"selection.get","Leggi selezione utente",{"Seleziona"},std::nullopt,"{}; solo sessione con viewport nativo, nessuna selezione headless",{},
        [](Session& s,const J& p)->CommandResult{only_keys(p,{});if(!s.native_view)return CommandResult::failure("selection.get richiede l'app aperta (bridge)");s.selection=canonical_selection(s.document,s.selection,true);return {true,{{"refs",s.selection}}, {}};},false}};}
}
