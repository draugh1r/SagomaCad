#include "kernel/topology.hpp"
#include "regen/hole_feature.hpp"
#include "doc/expression.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace sagomacad::regen {
namespace {
double length(const doc::Document& d,const nlohmann::json& expression){doc::Quantity q;std::string error;if(!expression.is_string()||!doc::evaluate_expression(d,expression.get<std::string>(),q,error))throw std::runtime_error("Quota foro: "+error);
    if((q.dimension!=doc::Dimension::Length&&q.dimension!=doc::Dimension::Scalar)||!std::isfinite(q.value)||q.value<=0||q.value>1e7)throw std::runtime_error("Quota foro: lunghezza positiva richiesta");return q.value;}
}
kernel::Result<HolePlacement> hole_placement(const doc::Document& d,const nlohmann::json& data,const kernel::Shape& shape)noexcept{
    try{
        auto ref=data.at("face").get<doc::TopologyRef>();auto face=kernel::resolve(shape,ref,"face");if(!face)throw std::runtime_error(face.error);auto frame=kernel::face_frame(shape,face.value->id);if(!frame)throw std::runtime_error(frame.error);auto f=*frame.value;
        const auto& refs=data.at("references");if(!refs.is_array()||refs.size()!=2)throw std::runtime_error("Il foro richiede due quote rispetto agli spigoli");double a[2],b[2],c[2];
        for(int i=0;i<2;++i){auto edge_ref=refs[i].at("edge").get<doc::TopologyRef>();if(edge_ref.body!=ref.body)throw std::runtime_error("Riferimento del foro su un altro corpo");
            auto edge=kernel::resolve(shape,edge_ref,"edge");if(!edge)throw std::runtime_error(edge.error);auto e=*edge.value;
            if(e.signature.type!="line"||std::find(face.value->edges.begin(),face.value->edges.end(),e.id)==face.value->edges.end())throw std::runtime_error("La quota deve riferirsi a un bordo lineare della faccia: "+edge_ref.name);
            auto direction=e.signature.direction,center=e.signature.center;
            double dx=direction[0]*f.u.x+direction[1]*f.u.y+direction[2]*f.u.z,dy=direction[0]*f.v.x+direction[1]*f.v.y+direction[2]*f.v.z,n=std::hypot(dx,dy);if(n<1e-9)throw std::runtime_error("Spigolo non complanare");
            a[i]=-dy/n;b[i]=dx/n;double x=(center[0]-f.origin.x)*f.u.x+(center[1]-f.origin.y)*f.u.y+(center[2]-f.origin.z)*f.u.z,y=(center[0]-f.origin.x)*f.v.x+(center[1]-f.origin.y)*f.v.y+(center[2]-f.origin.z)*f.v.z;
            int side=refs[i].at("side").get<int>();if(side!=1&&side!=-1)throw std::runtime_error("Lato della quota non valido");c[i]=a[i]*x+b[i]*y+side*length(d,refs[i].at("distance"));
        }
        double determinant=a[0]*b[1]-a[1]*b[0];if(std::abs(determinant)<1e-8)throw std::runtime_error("Gli spigoli di posizione del foro sono paralleli");
        return kernel::Result<HolePlacement>::success({(c[0]*b[1]-c[1]*b[0])/determinant,(a[0]*c[1]-a[1]*c[0])/determinant,f,*face.value});
    }catch(const std::exception& e){return kernel::Result<HolePlacement>::failure(e.what());}catch(...){return kernel::Result<HolePlacement>::failure("Posizione del foro non risolta");}
}
Regenerator::Cached build_hole_feature(const doc::Document& d,const doc::Feature& f,const Snapshot& snapshot){
    try{auto body_id=f.data.at("body_id").get<std::string>();auto body=std::find_if(snapshot.bodies.begin(),snapshot.bodies.end(),[&](const auto& b){return b.id==body_id;});if(body==snapshot.bodies.end()||!body->exact)throw std::runtime_error("Corpo del foro assente: "+body_id);
        auto point=hole_placement(d,f.data,*body->exact);if(!point)throw std::runtime_error(point.error);bool through=f.data.at("through_all").get<bool>();auto result=kernel::hole(*body->exact,point.value->face.id,point.value->x,point.value->y,length(d,f.data.at("diameter")),through?1:length(d,f.data.at("depth")),through,f.id);return {result.value,result.error};
    }catch(const std::exception& e){return {{},e.what()};}catch(...){return {{},"Foro fallito"};}
}
}
