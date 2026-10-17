#include "kernel/measure.hpp"
#include "kernel/shape_impl.hpp"
#include <BRepExtrema_DistShapeShape.hxx>
#include <Standard_Failure.hxx>
#include <cmath>
#include <numbers>
namespace sagomacad::kernel {
Result<Measurement> measure(const Shape& a,const TopologyEntity& ar,const Shape& b,const TopologyEntity& br)noexcept{
    try{
        auto subshape=[](const Shape& s,const TopologyEntity& r){auto p=ShapeAccess::get(s);if(!p)throw std::runtime_error("Corpo da misurare assente");auto kind=r.signature.kind;
            if(kind!="face"&&kind!="edge"&&kind!="vertex")throw std::runtime_error("Entità da misurare non valida");TopTools_IndexedMapOfShape map;TopExp::MapShapes(p->shape,kind=="face"?TopAbs_FACE:kind=="edge"?TopAbs_EDGE:TopAbs_VERTEX,map);
            if(r.id<1||r.id>map.Extent())throw std::runtime_error("Entità da misurare fuori intervallo");return map(r.id);};
        BRepExtrema_DistShapeShape distance(subshape(a,ar),subshape(b,br));distance.Perform();if(!distance.IsDone())throw std::runtime_error("Misura della distanza fallita");Measurement result;result.distance=distance.Value();
        auto u=ar.signature.direction,v=br.signature.direction;double un=std::hypot(u[0],u[1],u[2]),vn=std::hypot(v[0],v[1],v[2]);
        bool ad=ar.signature.type=="line"||ar.signature.type=="plane",bd=br.signature.type=="line"||br.signature.type=="plane";
        if(ad&&bd&&un>1e-8&&vn>1e-8){double angle=std::acos(std::clamp(std::abs((u[0]*v[0]+u[1]*v[1]+u[2]*v[2])/(un*vn)),0.,1.))*180/std::numbers::pi;if(ar.signature.kind!=br.signature.kind)angle=90-angle;result.angle_degrees=angle;}
        return Result<Measurement>::success(result);
    }catch(const Standard_Failure& e){return Result<Measurement>::failure(e.GetMessageString()?e.GetMessageString():"Misura OCCT fallita");}catch(const std::exception& e){return Result<Measurement>::failure(e.what());}catch(...){return Result<Measurement>::failure("Misura fallita");}
}
}
