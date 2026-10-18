#include "engine/command.hpp"
#include "engine/cmds/validation.hpp"
#include "regen/regenerator.hpp"
#include "kernel/topology.hpp"
#include <algorithm>
namespace sagomacad::engine::cmds {
namespace {
using J=nlohmann::json;
doc::TopologyRef face_ref(Session& s,const J& input){
    auto ref=input.get<doc::TopologyRef>();regen::Regenerator regen;auto snapshot=regen.evaluate(s.document);
    auto body=std::find_if(snapshot.bodies.begin(),snapshot.bodies.end(),[&](const auto& b){return b.id==ref.body;});
    if(body==snapshot.bodies.end()||!body->exact)throw std::runtime_error("Corpo della faccia assente: "+ref.body);
    auto resolved=kernel::resolve(*body->exact,ref,"face");if(!resolved)throw std::runtime_error(resolved.error);
    if(resolved.value->signature.type!="plane")throw std::runtime_error("È richiesta una faccia piana");ref.signature=resolved.value->signature;return ref;
}
CommandResult create(Session& s,const J& p,const std::string& type){
    if(type=="feature.extrude")only_keys(p,{"face","sketch","profiles","distance","direction","extent","second_distance","operation","target_bodies","name"});
    else only_keys(p,{"sketch","profiles","axis","type","angle","operation","target_bodies","name"});
    J data=p;data.erase("name");
    if(p.contains("face")){
        if(type!="feature.extrude"||p.contains("sketch")||p.contains("profiles"))return CommandResult::failure("Scegli face oppure sketch e profiles");data["face"]=face_ref(s,p.at("face"));
    }else data["sketch"]=string_arg(p,"sketch",true,256);
    if(!p.contains("face")&&(!p.contains("profiles")||!p["profiles"].is_array()||p["profiles"].empty()||p["profiles"].size()>1024))return CommandResult::failure("profiles deve contenere le regioni selezionate da sketch.inspect");
    for(const auto* key:{"distance","second_distance"}){data[key]=p.contains(key)?string_arg(p,key,true,4096):"10 mm";}
    for(const auto& [key,fallback]:{std::pair("direction","one_side"),std::pair("extent","distance"),std::pair("operation","new")})data[key]=p.contains(key)?string_arg(p,key,true,32):fallback;
    if(type=="feature.revolve"){
        data.erase("distance");data.erase("second_distance");data.erase("direction");data.erase("extent");
        data["axis"]=string_arg(p,"axis",true,256);data["type"]=p.contains("type")?string_arg(p,"type",true,32):"angle";data["angle"]=p.contains("angle")?string_arg(p,"angle",true,4096):"360 deg";
    }
    const auto op=data["operation"].get<std::string>();if(op!="new"&&op!="join"&&op!="cut"&&op!="intersect")return CommandResult::failure("Operazione non valida: new, join, cut, intersect");
    auto targets=p.value("target_bodies",J::array());if(!targets.is_array()||targets.size()>1024)return CommandResult::failure("target_bodies deve essere una lista di corpi");
    if(op=="new"&&!targets.empty())return CommandResult::failure("Nuovo corpo non ammette corpi di destinazione");
    if(op!="new"&&targets.empty())return CommandResult::failure("Seleziona i corpi di destinazione");
    std::vector<std::string> seen;
    for(const auto& target:targets){if(!target.is_string())return CommandResult::failure("ID corpo non valido");auto id=target.get<std::string>();if(std::find(seen.begin(),seen.end(),id)!=seen.end())return CommandResult::failure("Corpo duplicato");seen.push_back(id);auto body=std::find_if(s.document.bodies.begin(),s.document.bodies.end(),[&](const auto& b){return b.id==id&&b.kind==doc::BodyKind::BRep;});if(body==s.document.bodies.end())return CommandResult::failure("Corpo di destinazione mancante: "+id);}
    data["target_bodies"]=targets;
    size_t n=s.document.timeline.size()+1;std::string id,body;do{id="feature-"+std::to_string(n);body="body-"+std::to_string(n++);}while(s.document.features.contains(id)||std::any_of(s.document.bodies.begin(),s.document.bodies.end(),[&](const auto& b){return b.id==body;}));
    const auto name=p.contains("name")?string_arg(p,"name",true,256):(type=="feature.extrude"?"Estrusione ":"Rivoluzione ")+std::to_string(n-1);
    data["body_id"]=op=="new"?body:targets[0].get<std::string>();
    doc::Feature f;f.id=id;f.type=type;f.name=name;f.data=data;s.document.features[id]=f;
    s.document.timeline.insert(s.document.timeline.begin()+std::min(s.document.rollback,s.document.timeline.size()),id);++s.document.rollback;
    if(op=="new")s.document.bodies.push_back({body,name});
    regen::Regenerator regen;const auto& result=regen.evaluate(s.document);
    auto row=std::find_if(result.features.begin(),result.features.end(),[&](const auto& r){return r.id==id;});
    if(row==result.features.end()||!row->error.empty())return CommandResult::failure(row==result.features.end()?"Feature non valutata":row->error);
    if(s.hide_sketch_after_feature&&data.contains("sketch"))s.document.features.at(data.at("sketch").get<std::string>()).data["visible"]=false;
    s.active_sketch.clear();return {true,{{"feature_id",id},{"body_id",data["body_id"]}}, {}};
}
}
CommandResult edit_model_feature(Session& s,const J& p,const std::string& id){
    only_keys(p,{"feature_id","id","params","name"});
    if(!p.contains("params")||!p["params"].is_object())return CommandResult::failure("params deve contenere i parametri della feature");
    auto& f=s.document.features.at(id);const auto& changes=p["params"];
    if(f.type=="feature.extrude")only_keys(changes,{"face","sketch","profiles","distance","direction","extent","second_distance","operation","target_bodies"});
    else only_keys(changes,{"sketch","profiles","axis","type","angle","operation","target_bodies"});
    const auto previous=f.data;
    if(changes.contains("face")){if(changes.contains("sketch")||changes.contains("profiles"))return CommandResult::failure("Sorgenti incompatibili");f.data.erase("sketch");f.data.erase("profiles");}
    else if(changes.contains("sketch"))f.data.erase("face");
    for(auto it=changes.begin();it!=changes.end();++it)f.data[it.key()]=it.value();
    auto op=f.data.at("operation").get<std::string>();if(op!="new"&&op!="join"&&op!="cut"&&op!="intersect")return CommandResult::failure("Operazione non valida");
    const auto targets=f.data.value("target_bodies",J::array());
    if(!targets.is_array()||targets.size()>1024||(op=="new"&&!targets.empty())||(op!="new"&&targets.empty()))return CommandResult::failure("Corpi di destinazione non validi");
    if(p.contains("name"))f.name=string_arg(p,"name",true,256);
    if(op!="new"){
        std::vector<std::string> seen;
        for(const auto& target:targets){if(!target.is_string())return CommandResult::failure("ID corpo non valido");auto b=target.get<std::string>();if(std::find(seen.begin(),seen.end(),b)!=seen.end())return CommandResult::failure("Corpo duplicato");seen.push_back(b);}
        f.data["body_id"]=targets[0];
    }else if(previous.value("operation",std::string("new"))!="new"){
        const auto body="body-"+id;f.data["body_id"]=body;s.document.bodies.push_back({body,f.name});
    }
    auto saved=s.document.rollback;const auto at=std::find(s.document.timeline.begin(),s.document.timeline.end(),id);s.document.rollback=std::size_t(at-s.document.timeline.begin());
    if(changes.contains("face"))f.data["face"]=face_ref(s,changes.at("face"));
    ++s.document.rollback;
    regen::Regenerator regen;const auto snapshot=regen.evaluate(s.document);s.document.rollback=saved;
    const auto& state=snapshot.features.back();if(!state.error.empty())return CommandResult::failure(state.error);
    if(previous.value("operation",std::string("new"))=="new"&&op!="new"){
        const auto old=previous.at("body_id").get<std::string>();
        s.document.bodies.erase(std::remove_if(s.document.bodies.begin(),s.document.bodies.end(),[&](const auto& b){return b.id==old;}),s.document.bodies.end());
    }
    return {true,{{"feature_id",id},{"body_id",f.data["body_id"]}}, {}};
}
std::vector<CommandSpec> model_specs(){return {{"feature.extrude","Estrudi",{"Solido","Crea"},"E",
    "{face?: {body,name,signature?}, sketch?: id, profiles?: [region_id|[entity_id,...],...], distance?: expression, direction?: one_side|symmetric|two_sides, second_distance?: expression, extent?: distance|through_all, operation?: new|join|cut|intersect, target_bodies?: [id,...], name?: string}",{},
    [](Session& s,const J& p){return create(s,p,"feature.extrude");},true},
    {"feature.revolve","Rivoluzione",{"Solido","Crea"},std::nullopt,
    "{sketch: id, profiles: [region_id|[entity_id,...],...], axis: X|Y|Z|sketch_line_id, type?: angle|full, angle?: expression, operation?: new|join|cut|intersect, target_bodies?: [id,...], name?: string}",{},
    [](Session& s,const J& p){return create(s,p,"feature.revolve");},true},
    {"timeline.rollback","Marker di ritorno",{"Timeline"},std::nullopt,"{index: integer, 0..timeline.size}",{},
    [](Session& s,const J& p)->CommandResult{only_keys(p,{"index"});if(!p.contains("index")||!p["index"].is_number_integer())return CommandResult::failure("index deve essere un intero");auto index=p["index"].get<int64_t>();if(index<0||std::uint64_t(index)>s.document.timeline.size())return CommandResult::failure("Indice del marker fuori intervallo");s.document.rollback=std::size_t(index);s.active_sketch.clear();return {true,{{"index",index}}, {}};},true},
    {"feature.get","Ispeziona feature",{"Timeline"},std::nullopt,"{feature_id: id}; dati, errore, stato del marker",{},
    [](Session& s,const J& p)->CommandResult{
        only_keys(p,{"feature_id","id"});auto id=string_arg(p,p.contains("feature_id")?"feature_id":"id",true,256);auto f=s.document.features.find(id);if(f==s.document.features.end())return CommandResult::failure("Feature mancante: "+id);
        regen::Regenerator regen;const auto& snapshot=regen.evaluate(s.document);auto state=std::find_if(snapshot.features.begin(),snapshot.features.end(),[&](const auto& r){return r.id==id;});
        return {true,{{"feature_id",id},{"name",f->second.name},{"type",f->second.type},{"params",f->second.data},{"suppressed",f->second.suppressed},{"applied",state!=snapshot.features.end()&&!f->second.suppressed&&state->error.empty()},{"error",state==snapshot.features.end()?"":state->error}}, {}};
    },false}};}
}
