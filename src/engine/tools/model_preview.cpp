#include "engine/tools/model_preview.hpp"
#include "regen/model_features.hpp"
#include "kernel/topology.hpp"
#include "regen/attached_sketch.hpp"
#include "doc/expression.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
namespace sagomacad::engine {
namespace {
kernel::Vec3 world(const std::string& plane,double x,double y){return plane=="XY"?kernel::Vec3{x,y,0}:plane=="XZ"?kernel::Vec3{x,0,y}:kernel::Vec3{0,x,y};}
kernel::Vec3 world(const doc::Sketch& s,double x,double y){if(!s.support)return world(s.plane,x,y);return {s.origin[0]+s.u[0]*x+s.v[0]*y,s.origin[1]+s.u[1]*x+s.v[1]*y,s.origin[2]+s.u[2]*x+s.v[2]*y};}
}
std::vector<ModelProfile> model_profiles(const solver::SketchResult& solved){
    std::vector<ModelProfile> views;
    for(const auto& r:solver::profile_regions(solved.sketch,solved.profiles)){
        auto face=kernel::profile_face(solved.sketch,r.boundary,r.holes);if(!face)continue;auto fill=kernel::tessellate(*face.value,.1);if(!fill)continue;
        auto lo=r.polygon.front(),hi=lo;for(auto p:r.polygon)for(int i=0;i<2;++i){lo[i]=std::min(lo[i],p[i]);hi[i]=std::max(hi[i],p[i]);}
        double x=(lo[0]+hi[0])/2,y=(lo[1]+hi[1])/2;
        if(!solver::contains(r,x,y)){const double cx=x,cy=y;double best=1e100;for(int i=1;i<20;++i)for(int j=1;j<20;++j){double a=lo[0]+(hi[0]-lo[0])*i/20,b=lo[1]+(hi[1]-lo[1])*j/20,d=std::hypot(a-cx,b-cy);if(d<best&&solver::contains(r,a,b)){best=d;x=a;y=b;}}}
        views.push_back({r,*fill.value,world(solved.sketch,x,y)});
    }
    return views;
}
doc::Document model_context(const doc::Document& document,const std::string& editing){auto d=document;if(!editing.empty()){auto at=std::find(d.timeline.begin(),d.timeline.end(),editing);if(at!=d.timeline.end())d.rollback=std::size_t(at-d.timeline.begin());}return d;}
double model_drag_value(double start,double dx,double dy,double px,double py,bool angle){
    const double denominator=px*px+py*py;if(denominator<1e-8)return start;
    const double value=start+(dx*px+dy*py)/denominator;
    return angle?std::clamp(value,1.,360.):std::clamp(value,-1e6,1e6);
}
ModelPreview model_preview(const doc::Document& document,const std::string& editing,const std::string& type,const nlohmann::json& params){
    ModelPreview out;
    try{
        auto d=model_context(document,editing);regen::Regenerator regen;const auto snapshot=regen.evaluate(d);
        if(params.contains("face")){
            auto ref=params.at("face").get<doc::TopologyRef>();auto body=std::find_if(snapshot.bodies.begin(),snapshot.bodies.end(),[&](const auto& b){return b.id==ref.body;});if(body==snapshot.bodies.end()||!body->exact)throw std::runtime_error("Corpo della faccia assente");auto face=kernel::resolve(*body->exact,ref,"face");if(!face)throw std::runtime_error(face.error);auto frame=kernel::face_frame(*body->exact,face.value->id);if(!frame)throw std::runtime_error(frame.error);auto c=face.value->signature.center;out.handle={c[0],c[1],c[2]};out.normal=frame.value->normal;
            doc::Feature f;f.id="preview";f.type=type;f.data=params;auto built=regen::build_model_feature(d,f,snapshot);if(!built.shape)throw std::runtime_error(built.error);auto surface=mesh::build(*built.shape,.15);if(!surface)throw std::runtime_error(surface.error);out.surface=*surface.value;doc::Quantity q;std::string error;if(!doc::evaluate_expression(d,params.at("distance").get<std::string>(),q,error))throw std::runtime_error(error);out.amount=q.value;return out;
        }
        auto sk=params.at("sketch").get<std::string>();auto found=d.features.find(sk);if(found==d.features.end())throw std::runtime_error("Seleziona un profilo nel viewport");
        auto solved=solver::solve(d,regen::evaluated_sketch(d,sk));auto profiles=model_profiles(solved);
        for(const auto& p:profiles)if(std::find(params.at("profiles").begin(),params.at("profiles").end(),nlohmann::json(p.region.id))!=params.at("profiles").end()){out.handle=p.handle;break;}
        doc::Feature f;f.id="preview";f.type=type;f.data=params;
        const auto built=regen::build_model_feature(d,f,snapshot);if(!built.shape)throw std::runtime_error(built.error);
        auto surface=mesh::build(*built.shape,.15);if(!surface)throw std::runtime_error(surface.error);out.surface=*surface.value;
        const auto& plane=solved.sketch;out.normal=plane.support?kernel::Vec3{plane.u[1]*plane.v[2]-plane.u[2]*plane.v[1],plane.u[2]*plane.v[0]-plane.u[0]*plane.v[2],plane.u[0]*plane.v[1]-plane.u[1]*plane.v[0]}:solved.sketch.plane=="XY"?kernel::Vec3{0,0,1}:solved.sketch.plane=="XZ"?kernel::Vec3{0,-1,0}:kernel::Vec3{1,0,0};
        if(type=="feature.extrude"){
            doc::Quantity q;std::string error;if(!doc::evaluate_expression(d,params.at("distance").get<std::string>(),q,error))throw std::runtime_error(error);out.amount=q.value;
        }else {
            auto axis=params.at("axis").get<std::string>();if(axis=="X")out.axis_direction={1,0,0};else if(axis=="Y")out.axis_direction={0,1,0};else if(axis=="Z")out.axis_direction={0,0,1};
            else for(const auto& line:solved.sketch.entities)if(line.id==axis&&line.kind=="line"){
                auto point=[&](const std::string& id){for(const auto& p:solved.sketch.points)if(p.id==id)return world(solved.sketch,p.x,p.y);return kernel::Vec3{};};
                out.axis_origin=point(line.points[0]);auto b=point(line.points[1]);auto a=out.axis_origin;out.axis_direction={b.x-a.x,b.y-a.y,b.z-a.z};
            }
            doc::Quantity q;std::string error;if(params.value("type",std::string("angle"))!="full"&&!doc::evaluate_expression(d,params.at("angle").get<std::string>(),q,error))throw std::runtime_error(error);
            out.amount=params.value("type",std::string("angle"))=="full"?360:q.dimension==doc::Dimension::Angle?q.value*180/std::numbers::pi:q.value;
            auto a=out.axis_direction;double length=std::hypot(a.x,a.y,a.z);if(length<1e-8)throw std::runtime_error("Seleziona un asse");a={a.x/length,a.y/length,a.z/length};out.axis_direction=a;
            auto o=out.axis_origin;auto h=out.handle;kernel::Vec3 v{h.x-o.x,h.y-o.y,h.z-o.z};double dot=v.x*a.x+v.y*a.y+v.z*a.z;
            out.axis_origin={o.x+dot*a.x,o.y+dot*a.y,o.z+dot*a.z};v={h.x-out.axis_origin.x,h.y-out.axis_origin.y,h.z-out.axis_origin.z};
            for(int i=0;i<=72;++i){double angle=out.amount*std::numbers::pi/180*i/72,c=std::cos(angle),s=std::sin(angle);out.angle_arc.push_back({out.axis_origin.x+v.x*c+(a.y*v.z-a.z*v.y)*s,out.axis_origin.y+v.y*c+(a.z*v.x-a.x*v.z)*s,out.axis_origin.z+v.z*c+(a.x*v.y-a.y*v.x)*s});}
            auto endpoint=out.angle_arc.back();v={endpoint.x-out.axis_origin.x,endpoint.y-out.axis_origin.y,endpoint.z-out.axis_origin.z};const double degrees=std::numbers::pi/180;
            out.normal={(a.y*v.z-a.z*v.y)*degrees,(a.z*v.x-a.x*v.z)*degrees,(a.x*v.y-a.y*v.x)*degrees};
        }
        if(params.value("operation",std::string("new"))!="new"){
            const auto& targets=params.at("target_bodies");if(targets.empty())throw std::runtime_error("Seleziona i corpi di destinazione");
            for(const auto& target:targets){auto body=std::find_if(snapshot.bodies.begin(),snapshot.bodies.end(),[&](const auto& b){return target.is_string()&&b.id==target.get<std::string>();});if(body==snapshot.bodies.end()||!body->exact)throw std::runtime_error("Corpo di destinazione non disponibile");auto combined=kernel::boolean_operation(*body->exact,*built.shape,params.at("operation").get<std::string>());if(!combined)throw std::runtime_error(combined.error);}
        }
    }catch(const std::exception& e){out.error=e.what();}
    return out;
}
}
