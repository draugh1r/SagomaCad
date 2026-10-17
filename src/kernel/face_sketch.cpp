#include "kernel/topology.hpp"
#include "kernel/shape_impl.hpp"
#include <BRepAdaptor_Curve.hxx>
#include <BRep_Tool.hxx>
#include <Standard_Failure.hxx>
#include <TopoDS.hxx>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <sstream>
namespace sagomacad::kernel {
Result<doc::Sketch> face_reference_geometry(const Shape& source,int id)noexcept{
    try{
        auto frame=face_frame(source,id);if(!frame)throw std::runtime_error(frame.error);
        auto face=face_shape(source,id);if(!face)throw std::runtime_error(face.error);
        const auto& p=ShapeAccess::get(source);auto entities=topology(source);if(!entities)throw std::runtime_error(entities.error);
        auto f=*frame.value;doc::Sketch result;result.origin={f.origin.x,f.origin.y,f.origin.z};result.u={f.u.x,f.u.y,f.u.z};result.v={f.v.x,f.v.y,f.v.z};
        TopTools_IndexedMapOfShape all_edges,edges;TopExp::MapShapes(p->shape,TopAbs_EDGE,all_edges);TopExp::MapShapes(ShapeAccess::get(*face.value)->shape,TopAbs_EDGE,edges);
        auto local=[&](gp_Pnt q){double x=q.X()-f.origin.x,y=q.Y()-f.origin.y,z=q.Z()-f.origin.z;return std::pair(x*f.u.x+y*f.u.y+z*f.u.z,x*f.v.x+y*f.v.y+z*f.v.z);};
        for(int i=1;i<=edges.Extent();++i){
            const auto edge=TopoDS::Edge(edges(i));if(BRep_Tool::Degenerated(edge))continue;
            auto it=std::find_if(entities.value->begin(),entities.value->end(),[&](const auto& e){return e.signature.kind=="edge"&&e.id==all_edges.FindIndex(edge);});
            if(it==entities.value->end()||it->name.empty())throw std::runtime_error("Spigolo proiettato senza nome stabile");
            std::uint64_t hash=14695981039346656037ULL;for(unsigned char c:it->name){hash^=c;hash*=1099511628211ULL;}std::ostringstream key;key<<std::hex<<hash;auto suffix=key.str();
            doc::SketchEntity entity;entity.id="refe-"+suffix;entity.construction=true;entity.reference_name=it->name;
            if(std::any_of(result.entities.begin(),result.entities.end(),[&](const auto& e){return e.id==entity.id;}))throw std::runtime_error("Proiezione topologica ambigua: "+it->name);
            auto point=[&](gp_Pnt q){auto [x,y]=local(q);auto pid="refp-"+suffix+"-"+std::to_string(entity.points.size());result.points.push_back({pid,x,y});entity.points.push_back(pid);};
            BRepAdaptor_Curve c(edge);double a=c.FirstParameter(),b=c.LastParameter();
            if(c.GetType()==GeomAbs_Line){entity.kind="line";point(c.Value(a));point(c.Value(b));}
            else if(c.GetType()==GeomAbs_Circle){auto circle=c.Circle();entity.radius=circle.Radius();point(circle.Location());
                if(std::abs(b-a-2*std::numbers::pi)<1e-7)entity.kind="circle";
                else{entity.kind="arc";auto start=c.Value(a),end=c.Value(b);auto normal=circle.Axis().Direction();if(normal.X()*f.normal.x+normal.Y()*f.normal.y+normal.Z()*f.normal.z<0)std::swap(start,end);point(start);point(end);}
            }else throw std::runtime_error("Proiezione del bordo non supportata: "+it->name);
            result.entities.push_back(std::move(entity));
        }
        return Result<doc::Sketch>::success(std::move(result));
    }catch(const Standard_Failure& e){return Result<doc::Sketch>::failure(e.GetMessageString()?e.GetMessageString():"Proiezione fallita");}catch(const std::exception& e){return Result<doc::Sketch>::failure(e.what());}catch(...){return Result<doc::Sketch>::failure("Proiezione fallita");}
}
}
