#include "engine/command.hpp"
#include "engine/cmds/validation.hpp"
#include "regen/regenerator.hpp"
#include "kernel/topology.hpp"
#include <algorithm>
#include <set>
namespace sagomacad::engine::cmds {
namespace {
using J=nlohmann::json;
J canonical_edges(Session& s,const J& input){
    if(!input.is_array()||input.empty()||input.size()>4096)throw std::runtime_error("edges deve contenere uno o più riferimenti stabili");
    regen::Regenerator r;auto snapshot=r.evaluate(s.document);J output=J::array();std::set<std::pair<std::string,int>> seen;std::string owner;
    for(const auto& j:input){auto ref=j.get<doc::TopologyRef>();if(owner.empty())owner=ref.body;if(ref.body!=owner)throw std::runtime_error("Gli spigoli devono appartenere allo stesso corpo");
        auto b=std::find_if(snapshot.bodies.begin(),snapshot.bodies.end(),[&](const auto& b){return b.id==ref.body;});if(b==snapshot.bodies.end()||!b->exact)throw std::runtime_error("Corpo sorgente assente: "+ref.body);
        auto edge=kernel::resolve(*b->exact,ref,"edge");if(!edge)throw std::runtime_error(edge.error);if(!seen.insert({ref.body,edge.value->id}).second)throw std::runtime_error("Spigolo duplicato: "+ref.name);
        ref.signature=edge.value->signature;output.push_back(ref);
    }return output;
}
CommandResult validate(Session& s,const std::string& id){regen::Regenerator r;auto snapshot=r.evaluate(s.document);auto row=std::find_if(snapshot.features.begin(),snapshot.features.end(),[&](const auto& f){return f.id==id;});
    if(row==snapshot.features.end()||!row->error.empty())return CommandResult::failure(row==snapshot.features.end()?"Feature non valutata":row->error);return {true,{{"feature_id",id},{"body_id",s.document.features.at(id).data.at("body_id")}}, {}};
}
CommandResult create(Session& s,const J& p,bool round){
    only_keys(p,round?std::initializer_list<std::string_view>{"edges","radius","name"}:std::initializer_list<std::string_view>{"edges","distance","name"});
    J data;data["edges"]=canonical_edges(s,p.at("edges"));const char* key=round?"radius":"distance";data[key]=string_arg(p,key,true,4096);data["body_id"]=data["edges"][0]["body"];
    size_t n=s.document.timeline.size()+1;std::string id;do{id="feature-"+std::to_string(n++);}while(s.document.features.contains(id));doc::Feature f;f.id=id;f.type=round?"feature.fillet":"feature.chamfer";f.name=p.contains("name")?string_arg(p,"name",true,256):(round?"Raccordo ":"Smusso ")+std::to_string(n-1);f.data=data;
    s.document.features[id]=f;s.document.timeline.insert(s.document.timeline.begin()+std::min(s.document.rollback,s.document.timeline.size()),id);++s.document.rollback;return validate(s,id);
}
}
CommandResult edit_surface_feature(Session& s,const J& p,const std::string& id){
    only_keys(p,{"feature_id","id","params","name"});if(!p.contains("params")||!p["params"].is_object())return CommandResult::failure("params deve essere un oggetto");
    auto& f=s.document.features.at(id);bool round=f.type=="feature.fillet";const auto& changes=p.at("params");
    only_keys(changes,round?std::initializer_list<std::string_view>{"edges","radius"}:std::initializer_list<std::string_view>{"edges","distance"});
    auto saved=s.document.rollback;auto at=std::find(s.document.timeline.begin(),s.document.timeline.end(),id);s.document.rollback=std::size_t(at-s.document.timeline.begin());
    try{
        if(changes.contains("edges")){f.data["edges"]=canonical_edges(s,changes.at("edges"));f.data["body_id"]=f.data["edges"][0]["body"];}
        const char* key=round?"radius":"distance";if(changes.contains(key))f.data[key]=string_arg(changes,key,true,4096);if(p.contains("name"))f.name=string_arg(p,"name",true,256);
        ++s.document.rollback;auto result=validate(s,id);s.document.rollback=saved;return result;
    }catch(...){s.document.rollback=saved;throw;}
}
std::vector<CommandSpec> surface_specs(){return {
    {"feature.fillet","Raccordo",{"Solido","Modifica"},"F","{edges: [{body,name,signature?},...], radius: expression, name?: string}",{},[](Session& s,const J& p){return create(s,p,true);},true},
    {"feature.chamfer","Smusso",{"Solido","Modifica"},std::nullopt,"{edges: [{body,name,signature?},...], distance: expression, name?: string}",{},[](Session& s,const J& p){return create(s,p,false);},true}};}
}
