#include "kernel/topology.hpp"
#include "kernel/shape_impl.hpp"
#include <BRepClass_FaceClassifier.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <Standard_Failure.hxx>
#include <TopoDS.hxx>
#include <gp_Ax2.hxx>
#include <cmath>
namespace sagomacad::kernel {
Result<Shape> hole(const Shape& source,int face_id,double x,double y,double diameter,double depth,bool through,const std::string& origin)noexcept{
    try{
        if(!std::isfinite(x)||!std::isfinite(y)||std::abs(x)>1e7||std::abs(y)>1e7||!std::isfinite(diameter)||diameter<=0||diameter>1e7||(!through&&(!std::isfinite(depth)||depth<=0||depth>1e7)))throw std::runtime_error("Posizione, diametro o profondità non validi");
        auto frame=face_frame(source,face_id);if(!frame)throw std::runtime_error(frame.error);auto face=face_shape(source,face_id);if(!face)throw std::runtime_error(face.error);const auto& f=*frame.value;
        gp_Pnt point(f.origin.x+x*f.u.x+y*f.v.x,f.origin.y+x*f.u.y+y*f.v.y,f.origin.z+x*f.u.z+y*f.v.z);
        BRepClass_FaceClassifier classifier(TopoDS::Face(ShapeAccess::get(*face.value)->shape),point,1e-7);if(classifier.State()!=TopAbs_IN&&classifier.State()!=TopAbs_ON)throw std::runtime_error("Il centro del foro è fuori dalla faccia selezionata");
        auto mass=mass_properties(source);if(!mass)throw std::runtime_error(mass.error);
        auto lo=mass.value->minimum,hi=mass.value->maximum;double span=std::hypot(hi.x-lo.x,hi.y-lo.y,hi.z-lo.z),epsilon=1e-5;
        if(through)depth=span+1;point.Translate(gp_Vec(f.normal.x*epsilon,f.normal.y*epsilon,f.normal.z*epsilon));
        BRepPrimAPI_MakeCylinder cylinder(gp_Ax2(point,gp_Dir(-f.normal.x,-f.normal.y,-f.normal.z)),diameter/2,depth+epsilon);cylinder.Build();if(!cylinder.IsDone())throw std::runtime_error("Utensile del foro non costruito");
        auto cutter=name_topology(ShapeAccess::make(cylinder.Shape()),origin);if(!cutter)throw std::runtime_error(cutter.error);
        auto result=subtract(source,*cutter.value);if(!result)throw std::runtime_error(result.error);if(!BRepCheck_Analyzer(ShapeAccess::get(*result.value)->shape).IsValid())throw std::runtime_error("Foro OCCT non valido");
        return name_topology(*result.value,origin);
    }catch(const Standard_Failure& e){return Result<Shape>::failure(std::string("Foro: ")+(e.GetMessageString()?e.GetMessageString():"operazione OCCT fallita"));}catch(const std::exception& e){return Result<Shape>::failure(std::string("Foro: ")+e.what());}catch(...){return Result<Shape>::failure("Foro: operazione fallita");}
}
}
