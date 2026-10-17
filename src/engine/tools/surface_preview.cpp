#include "engine/tools/surface_preview.hpp"
#include "engine/tools/model_preview.hpp"
#include "regen/hole_feature.hpp"
#include "doc/expression.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
namespace sagomacad::engine {
namespace {
kernel::Vec3 add(kernel::Vec3 a,kernel::Vec3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
kernel::Vec3 mul(kernel::Vec3 a,double k){return {a.x*k,a.y*k,a.z*k};}
double dot(kernel::Vec3 a,kernel::Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
kernel::Vec3 unit(kernel::Vec3 a){double n=std::sqrt(dot(a,a));return n>1e-9?mul(a,1/n):kernel::Vec3{1,0,0};}
double length(const doc::Document& d,const nlohmann::json& p,const char* key){doc::Quantity q;std::string error;if(!doc::evaluate_expression(d,p.at(key).get<std::string>(),q,error))throw std::runtime_error(error);return q.value;}
}
SurfacePreview surface_preview(const doc::Document& document,const std::string& editing,const std::string& command,const nlohmann::json& params){
    SurfacePreview out;
    try{
        Engine copy;copy.session().document=model_context(document,editing);auto d=copy.session().document;regen::Regenerator r;auto before=r.evaluate(d);
        auto result=copy.execute(command,params);if(!result.ok)throw std::runtime_error(result.error);auto id=result.value.at("feature_id").get<std::string>();auto data=copy.session().document.features.at(id).data;out.params=data;out.params.erase("body_id");
        auto body_id=data.at("body_id").get<std::string>();out.source_body=body_id;auto original=std::find_if(before.bodies.begin(),before.bodies.end(),[&](const auto& b){return b.id==body_id;});if(original==before.bodies.end()||!original->exact)throw std::runtime_error("Corpo sorgente assente");
        auto snapshot=r.evaluate(copy.session().document);auto body=std::find_if(snapshot.bodies.begin(),snapshot.bodies.end(),[&](const auto& b){return b.id==body_id;});if(body==snapshot.bodies.end()||!body->exact)throw std::runtime_error("Anteprima assente");auto surface=mesh::build(*body->exact,.15);if(!surface)throw std::runtime_error(surface.error);out.surface=*surface.value;
        if(command=="feature.hole"){
            auto placement=regen::hole_placement(d,data,*original->exact);if(!placement)throw std::runtime_error(placement.error);auto p=*placement.value;auto f=p.frame;out.start={f.origin.x+f.u.x*p.x+f.v.x*p.y,f.origin.y+f.u.y*p.x+f.v.y*p.y,f.origin.z+f.u.z*p.x+f.v.z*p.y};out.normal=mul(f.normal,-1);out.amount=length(d,data,"depth");
            if(data.at("through_all").get<bool>()){auto mass=kernel::mass_properties(*original->exact);if(!mass)throw std::runtime_error(mass.error);auto lo=mass.value->minimum,hi=mass.value->maximum;out.amount=0;for(int i=0;i<8;++i)out.amount=std::max(out.amount,dot(add({i&1?hi.x:lo.x,i&2?hi.y:lo.y,i&4?hi.z:lo.z},mul(out.start,-1)),out.normal));}
            double radius=length(d,data,"diameter")/2;
            auto ring=[&](int i,bool bottom){double angle=2*std::numbers::pi*i/64;auto p=out.start;p=add(p,{f.u.x*radius*std::cos(angle)+f.v.x*radius*std::sin(angle),f.u.y*radius*std::cos(angle)+f.v.y*radius*std::sin(angle),f.u.z*radius*std::cos(angle)+f.v.z*radius*std::sin(angle)});if(bottom)p=add(p,mul(out.normal,out.amount));return p;};
            for(int i=0;i<64;++i){auto a=ring(i,false),b=ring(i+1,false),c=ring(i,true),e=ring(i+1,true);out.cutter.triangles.push_back({{out.start,a,b},f.normal,0});out.cutter.triangles.push_back({{a,c,b},f.normal,0});out.cutter.triangles.push_back({{b,c,e},f.normal,0});}
            for(const auto& input:data.at("references")){auto ref=input.at("edge").get<doc::TopologyRef>();auto e=kernel::resolve(*original->exact,ref,"edge");if(!e)throw std::runtime_error(e.error);auto c=e.value->signature.center,t=e.value->signature.direction;kernel::Vec3 center{c[0],c[1],c[2]},tangent{t[0],t[1],t[2]};auto delta=add(out.start,mul(center,-1));auto foot=add(center,mul(tangent,dot(delta,tangent)));out.dimensions.push_back({foot,out.start,unit(add(out.start,mul(foot,-1))),std::sqrt(dot(add(out.start,mul(foot,-1)),add(out.start,mul(foot,-1))))});}
        }else{
            auto ref=data.at("edges")[0].get<doc::TopologyRef>();auto edge=kernel::resolve(*original->exact,ref,"edge");if(!edge)throw std::runtime_error(edge.error);auto c=edge.value->signature.center,t=edge.value->signature.direction;out.start={c[0],c[1],c[2]};auto mass=kernel::mass_properties(*original->exact);if(!mass)throw std::runtime_error(mass.error);auto v=add(mass.value->center,mul(out.start,-1));kernel::Vec3 tangent{t[0],t[1],t[2]};out.normal=unit(add(v,mul(tangent,-dot(v,tangent))));out.amount=length(d,data,command=="feature.fillet"?"radius":"distance");
        }
        out.handle=add(out.start,mul(out.normal,out.amount));
    }catch(const std::exception& e){out.error=e.what();}
    return out;
}
kernel::Result<nlohmann::json> face_position(const doc::Document& document,const nlohmann::json& input,kernel::Vec3 origin,kernel::Vec3 direction)noexcept{
    try{auto ref=input.get<doc::TopologyRef>();regen::Regenerator r;auto snapshot=r.evaluate(document);auto body=std::find_if(snapshot.bodies.begin(),snapshot.bodies.end(),[&](const auto& b){return b.id==ref.body;});if(body==snapshot.bodies.end()||!body->exact)throw std::runtime_error("Corpo della faccia assente");auto face=kernel::resolve(*body->exact,ref,"face");if(!face)throw std::runtime_error(face.error);auto frame=kernel::face_frame(*body->exact,face.value->id);if(!frame)throw std::runtime_error(frame.error);auto f=*frame.value;double den=dot(direction,f.normal);if(std::abs(den)<1e-9)throw std::runtime_error("Vista parallela alla faccia");double t=dot(add(f.origin,mul(origin,-1)),f.normal)/den;if(!std::isfinite(t)||t<=0)throw std::runtime_error("Punto dietro la camera");auto delta=add(add(origin,mul(direction,t)),mul(f.origin,-1));return kernel::Result<nlohmann::json>::success({{"x",dot(delta,f.u)},{"y",dot(delta,f.v)}});
    }catch(const std::exception& e){return kernel::Result<nlohmann::json>::failure(e.what());}catch(...){return kernel::Result<nlohmann::json>::failure("Posizione sulla faccia non disponibile");}
}
}
