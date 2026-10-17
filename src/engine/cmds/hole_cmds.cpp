#include "engine/command.hpp"
#include "engine/cmds/validation.hpp"
#include "kernel/topology.hpp"
#include "regen/hole_feature.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <set>
namespace sagomacad::engine::cmds {
namespace {
using J=nlohmann::json;
J hole_data(Session& s,const J& p){
    only_keys(p,{"face","position","diameter","depth","through_all","references","name"});
    auto ref=p.at("face").get<doc::TopologyRef>();regen::Regenerator r;auto snapshot=r.evaluate(s.document);auto body=std::find_if(snapshot.bodies.begin(),snapshot.bodies.end(),[&](const auto& b){return b.id==ref.body;});
    if(body==snapshot.bodies.end()||!body->exact)throw std::runtime_error("Corpo della faccia assente: "+ref.body);
    const auto& shape=*body->exact;auto face=kernel::resolve(shape,ref,"face");if(!face)throw std::runtime_error(face.error);auto frame=kernel::face_frame(shape,face.value->id);if(!frame)throw std::runtime_error(frame.error);ref.signature=face.value->signature;auto f=*frame.value;
    auto local=[&](std::array<double,3> point){double x=point[0]-f.origin.x,y=point[1]-f.origin.y,z=point[2]-f.origin.z;return std::pair(x*f.u.x+y*f.u.y+z*f.u.z,x*f.v.x+y*f.v.y+z*f.v.z);};
    auto [x,y]=local(face.value->signature.center);
    if(p.contains("position")){const auto& point=p.at("position");if(!point.is_object())throw std::runtime_error("position deve essere {x,y} in mm");only_keys(point,{"x","y"});x=point.at("x").get<double>();y=point.at("y").get<double>();}
    else if(!p.contains("references"))throw std::runtime_error("Fai clic sulla faccia oppure indica position: {x,y}");
    if(!std::isfinite(x)||!std::isfinite(y)||std::abs(x)>1e7||std::abs(y)>1e7)throw std::runtime_error("Posizione del foro non valida");
    auto entities=kernel::topology(shape);if(!entities)throw std::runtime_error(entities.error);
    struct Candidate{kernel::TopologyEntity edge;double a,b,c,distance;};std::vector<Candidate> candidates;
    for(const auto& edge:*entities.value)if(edge.signature.kind=="edge"&&edge.signature.type=="line"&&std::find(face.value->edges.begin(),face.value->edges.end(),edge.id)!=face.value->edges.end()){
        auto d=edge.signature.direction;double dx=d[0]*f.u.x+d[1]*f.u.y+d[2]*f.u.z,dy=d[0]*f.v.x+d[1]*f.v.y+d[2]*f.v.z,n=std::hypot(dx,dy);if(n<1e-8)continue;
        auto [ex,ey]=local(edge.signature.center);double a=-dy/n,b=dx/n,c=a*ex+b*ey;candidates.push_back({edge,a,b,c,std::abs(a*x+b*y-c)});
    }
    std::stable_sort(candidates.begin(),candidates.end(),[](const auto& a,const auto& b){return a.distance<b.distance;});
    J inputs=p.value("references",J::array());
    if(!p.contains("references")){
        if(candidates.empty())throw std::runtime_error("Servono due bordi lineari per quotare il foro");
        auto second=std::find_if(candidates.begin()+1,candidates.end(),[&](const auto& c){return std::abs(c.a*candidates[0].b-c.b*candidates[0].a)>1e-8;});
        if(second==candidates.end())throw std::runtime_error("Servono due bordi non paralleli per quotare il foro");
        for(const auto* c:{&candidates[0],&*second}){char distance[64];std::snprintf(distance,sizeof(distance),"%.12g mm",c->distance);inputs.push_back({{"edge",doc::TopologyRef{ref.body,c->edge.name,c->edge.signature}},{"distance",distance}});}
    }
    if(!inputs.is_array()||inputs.size()!=2)throw std::runtime_error("references richiede due quote dai bordi");J references=J::array();std::set<int> seen;
    for(const auto& input:inputs){only_keys(input,{"edge","distance","side"});auto edge_ref=input.at("edge").get<doc::TopologyRef>();if(edge_ref.body!=ref.body)throw std::runtime_error("Riferimento su un altro corpo");auto edge=kernel::resolve(shape,edge_ref,"edge");if(!edge)throw std::runtime_error(edge.error);
        auto candidate=std::find_if(candidates.begin(),candidates.end(),[&](const auto& c){return c.edge.id==edge.value->id;});if(candidate==candidates.end()||!seen.insert(edge.value->id).second)throw std::runtime_error("Bordo di posizione non valido o duplicato: "+edge_ref.name);
        edge_ref.signature=edge.value->signature;int side=input.contains("side")?input.at("side").get<int>():(candidate->a*x+candidate->b*y-candidate->c<0?-1:1);
        references.push_back({{"edge",edge_ref},{"distance",string_arg(input,"distance",true,4096)},{"side",side}});
    }
    return {{"face",ref},{"body_id",ref.body},{"position",{{"x",x},{"y",y}}},{"diameter",string_arg(p,"diameter",true,4096)},{"depth",p.contains("depth")?string_arg(p,"depth",true,4096):"10 mm"},{"through_all",p.value("through_all",false)},{"references",references}};
}
CommandResult validate(Session& s,const std::string& id){regen::Regenerator r;auto snapshot=r.evaluate(s.document);auto found=std::find_if(snapshot.features.begin(),snapshot.features.end(),[&](const auto& f){return f.id==id;});if(found==snapshot.features.end()||!found->error.empty())return CommandResult::failure(found==snapshot.features.end()?"Foro non valutato":found->error);return {true,{{"feature_id",id},{"body_id",s.document.features.at(id).data.at("body_id")}}, {}};}
}
CommandResult edit_hole_feature(Session& s,const J& p,const std::string& id){
    only_keys(p,{"id","feature_id","params","name"});if(!p.contains("params")||!p.at("params").is_object())return CommandResult::failure("params deve essere un oggetto");auto& f=s.document.features.at(id);J data=f.data;data.erase("body_id");const auto& changes=p.at("params");
    only_keys(changes,{"face","position","diameter","depth","through_all","references"});if((changes.contains("position")||changes.contains("face"))&&!changes.contains("references"))data.erase("references");
    for(auto it=changes.begin();it!=changes.end();++it)data[it.key()]=it.value();auto saved=s.document.rollback;auto at=std::find(s.document.timeline.begin(),s.document.timeline.end(),id);s.document.rollback=std::size_t(at-s.document.timeline.begin());
    try{f.data=hole_data(s,data);if(p.contains("name"))f.name=string_arg(p,"name",true,256);++s.document.rollback;auto result=validate(s,id);s.document.rollback=saved;return result;}catch(...){s.document.rollback=saved;throw;}
}
std::vector<CommandSpec> hole_specs(){return {{"feature.hole","Foro",{"Solido","Crea"},"H","{face: {body,name,signature?}, position?: {x,y}, diameter: expression, depth?: expression, through_all?: bool, references?: [{edge:{body,name,signature?},distance:expression,side?:-1|1},...], name?: string}; due quote dai bordi, automatiche dal clic",{},
    [](Session& s,const J& p)->CommandResult{auto data=hole_data(s,p);size_t n=s.document.timeline.size()+1;std::string id;do{id="feature-"+std::to_string(n++);}while(s.document.features.contains(id));doc::Feature f;f.id=id;f.type="feature.hole";f.name=p.contains("name")?string_arg(p,"name",true,256):"Foro "+std::to_string(n-1);f.data=data;s.document.features[id]=f;s.document.timeline.insert(s.document.timeline.begin()+std::min(s.document.rollback,s.document.timeline.size()),id);++s.document.rollback;return validate(s,id);},true}};}
}
