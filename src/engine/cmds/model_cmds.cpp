#include "engine/command.hpp"
#include "engine/cmds/validation.hpp"
#include "regen/regenerator.hpp"
#include <algorithm>
namespace sagomacad::engine::cmds {
namespace {
using J=nlohmann::json;
CommandResult create(Session& s,const J& p,const std::string& type){
    only_keys(p,{"sketch","profiles","distance","direction","extent","second_distance","operation","target_bodies","name"});
    J data=p;data.erase("name");data["sketch"]=string_arg(p,"sketch",true,256);
    if(!p.contains("profiles")||!p["profiles"].is_array()||p["profiles"].empty()||p["profiles"].size()>1024)return CommandResult::failure("profiles deve contenere le regioni selezionate da sketch.inspect");
    for(const auto* key:{"distance","second_distance"}){data[key]=p.contains(key)?string_arg(p,key,true,4096):"10 mm";}
    for(const auto& [key,fallback]:{std::pair("direction","one_side"),std::pair("extent","distance"),std::pair("operation","new")})data[key]=p.contains(key)?string_arg(p,key,true,32):fallback;
    const auto op=data["operation"].get<std::string>();if(op!="new"&&op!="join"&&op!="cut"&&op!="intersect")return CommandResult::failure("Operazione non valida: new, join, cut, intersect");
    auto targets=p.value("target_bodies",J::array());if(!targets.is_array()||targets.size()>1024)return CommandResult::failure("target_bodies deve essere una lista di corpi");
    if(op=="new"&&!targets.empty())return CommandResult::failure("Nuovo corpo non ammette corpi di destinazione");
    if(op!="new"&&targets.empty())return CommandResult::failure("Seleziona i corpi di destinazione");
    std::vector<std::string> seen;
    for(const auto& target:targets){if(!target.is_string())return CommandResult::failure("ID corpo non valido");auto id=target.get<std::string>();if(std::find(seen.begin(),seen.end(),id)!=seen.end())return CommandResult::failure("Corpo duplicato");seen.push_back(id);auto body=std::find_if(s.document.bodies.begin(),s.document.bodies.end(),[&](const auto& b){return b.id==id&&b.kind==doc::BodyKind::BRep;});if(body==s.document.bodies.end())return CommandResult::failure("Corpo di destinazione mancante: "+id);}
    data["target_bodies"]=targets;
    size_t n=s.document.timeline.size()+1;std::string id,body;do{id="feature-"+std::to_string(n);body="body-"+std::to_string(n++);}while(s.document.features.contains(id)||std::any_of(s.document.bodies.begin(),s.document.bodies.end(),[&](const auto& b){return b.id==body;}));
    const auto name=p.contains("name")?string_arg(p,"name",true,256):"Estrusione "+std::to_string(n-1);
    data["body_id"]=op=="new"?body:targets[0].get<std::string>();
    doc::Feature f;f.id=id;f.type=type;f.name=name;f.data=data;s.document.features[id]=f;
    s.document.timeline.insert(s.document.timeline.begin()+std::min(s.document.rollback,s.document.timeline.size()),id);++s.document.rollback;
    if(op=="new")s.document.bodies.push_back({body,name});
    regen::Regenerator regen;const auto& result=regen.evaluate(s.document);
    auto row=std::find_if(result.features.begin(),result.features.end(),[&](const auto& r){return r.id==id;});
    if(row==result.features.end()||!row->error.empty())return CommandResult::failure(row==result.features.end()?"Feature non valutata":row->error);
    s.active_sketch.clear();return {true,{{"feature_id",id},{"body_id",data["body_id"]}}, {}};
}
}
std::vector<CommandSpec> model_specs(){return {{"feature.extrude","Estrudi",{"Solido","Crea"},"E",
    "{sketch: id, profiles: [region_id|[entity_id,...],...], distance?: expression, direction?: one_side|symmetric|two_sides, second_distance?: expression, extent?: distance|through_all, operation?: new|join|cut|intersect, target_bodies?: [id,...], name?: string}",{},
    [](Session& s,const J& p){return create(s,p,"feature.extrude");},true}};}
}
