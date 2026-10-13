#include "kernel/shape.hpp"
#include <BRepGProp.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <GProp_GProps.hxx>
#include <Standard_Failure.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <cmath>
#include <exception>

namespace sagomacad::kernel {
struct Shape::Impl { TopoDS_Shape shape; };
namespace {
bool positive(double value) { return std::isfinite(value) && value>0.0 && value<=1.0e9; }
std::string occt_message(const Standard_Failure& failure) {
    const char* text=failure.GetMessageString();
    return text && *text ? std::string(text) : "Operazione OpenCascade fallita";
}
}
Result<Shape> make_box(double width,double depth,double height) noexcept {
    if (!positive(width) || !positive(depth) || !positive(height))
        return Result<Shape>::failure("Dimensioni del box non valide");
    try {
        BRepPrimAPI_MakeBox builder(width,depth,height);
        const auto shape=builder.Shape();
        if (!builder.IsDone()) return Result<Shape>::failure("Creazione del box fallita");
        return Result<Shape>::success(Shape(std::make_shared<Shape::Impl>(shape)));
    } catch (const Standard_Failure& e) { return Result<Shape>::failure(occt_message(e)); }
      catch (const std::exception& e) { return Result<Shape>::failure(e.what()); }
      catch (...) { return Result<Shape>::failure("Errore imprevisto nella creazione del box"); }
}
Result<Shape> make_cylinder(double radius,double height) noexcept {
    if (!positive(radius) || !positive(height))
        return Result<Shape>::failure("Dimensioni del cilindro non valide");
    try {
        BRepPrimAPI_MakeCylinder builder(radius,height);
        const auto shape=builder.Shape();
        if (!builder.IsDone()) return Result<Shape>::failure("Creazione del cilindro fallita");
        return Result<Shape>::success(Shape(std::make_shared<Shape::Impl>(shape)));
    } catch (const Standard_Failure& e) { return Result<Shape>::failure(occt_message(e)); }
      catch (const std::exception& e) { return Result<Shape>::failure(e.what()); }
      catch (...) { return Result<Shape>::failure("Errore imprevisto nella creazione del cilindro"); }
}
Result<Shape> make_sphere(double radius) noexcept {
    if (!positive(radius)) return Result<Shape>::failure("Raggio della sfera non valido");
    try {
        BRepPrimAPI_MakeSphere builder(radius);
        const auto shape=builder.Shape();
        if (!builder.IsDone()) return Result<Shape>::failure("Creazione della sfera fallita");
        return Result<Shape>::success(Shape(std::make_shared<Shape::Impl>(shape)));
    } catch (const Standard_Failure& e) { return Result<Shape>::failure(occt_message(e)); }
      catch (const std::exception& e) { return Result<Shape>::failure(e.what()); }
      catch (...) { return Result<Shape>::failure("Errore imprevisto nella creazione della sfera"); }
}
Result<MassProperties> mass_properties(const Shape& shape) noexcept {
    if (!shape.impl_) return Result<MassProperties>::failure("Forma vuota");
    try {
        GProp_GProps volume,area;
        BRepGProp::VolumeProperties(shape.impl_->shape,volume);
        BRepGProp::SurfaceProperties(shape.impl_->shape,area);
        const MassProperties result{volume.Mass(),area.Mass()};
        if (!std::isfinite(result.volume) || !std::isfinite(result.area) || result.volume<=0.0 || result.area<=0.0)
            return Result<MassProperties>::failure("Proprietà geometriche non valide");
        return Result<MassProperties>::success(result);
    } catch (const Standard_Failure& e) { return Result<MassProperties>::failure(occt_message(e)); }
      catch (const std::exception& e) { return Result<MassProperties>::failure(e.what()); }
      catch (...) { return Result<MassProperties>::failure("Errore imprevisto nel calcolo delle proprietà"); }
}
Result<Shape> translate(const Shape& shape,double x,double y,double z) noexcept {
    if (!shape.impl_) return Result<Shape>::failure("Forma vuota");
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z) ||
        std::abs(x)>1.0e9 || std::abs(y)>1.0e9 || std::abs(z)>1.0e9)
        return Result<Shape>::failure("Traslazione non valida");
    if (x==0.0 && y==0.0 && z==0.0) return Result<Shape>::success(shape);
    try {
        gp_Trsf transform;
        transform.SetTranslation(gp_Vec(x,y,z));
        BRepBuilderAPI_Transform builder(shape.impl_->shape,transform,true);
        const auto moved=builder.Shape();
        if (!builder.IsDone()) return Result<Shape>::failure("Traslazione fallita");
        return Result<Shape>::success(Shape(std::make_shared<Shape::Impl>(moved)));
    } catch (const Standard_Failure& e) { return Result<Shape>::failure(occt_message(e)); }
      catch (const std::exception& e) { return Result<Shape>::failure(e.what()); }
      catch (...) { return Result<Shape>::failure("Errore imprevisto nella traslazione"); }
}
} // namespace sagomacad::kernel
