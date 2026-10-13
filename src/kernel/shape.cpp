#include "kernel/shape.hpp"
#include <BRepGProp.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <GProp_GProps.hxx>
#include <Standard_Failure.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopAbs_Orientation.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopLoc_Location.hxx>
#include <Poly_Triangulation.hxx>
#include <Poly_Triangle.hxx>
#include <GeomAbs_CurveType.hxx>
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
        Bnd_Box bounds;
        BRepBndLib::Add(shape.impl_->shape,bounds);
        if (bounds.IsVoid()) return Result<MassProperties>::failure("Bounding box non valida");
        double xmin=0,ymin=0,zmin=0,xmax=0,ymax=0,zmax=0;
        bounds.Get(xmin,ymin,zmin,xmax,ymax,zmax);
        const auto center=volume.CentreOfMass();
        const MassProperties result{volume.Mass(),area.Mass(),
            {center.X(),center.Y(),center.Z()},{xmin,ymin,zmin},{xmax,ymax,zmax}};
        if (!std::isfinite(result.volume) || !std::isfinite(result.area) || result.volume<=0.0 || result.area<=0.0)
            return Result<MassProperties>::failure("Proprietà geometriche non valide");
        return Result<MassProperties>::success(result);
    } catch (const Standard_Failure& e) { return Result<MassProperties>::failure(occt_message(e)); }
      catch (const std::exception& e) { return Result<MassProperties>::failure(e.what()); }
      catch (...) { return Result<MassProperties>::failure("Errore imprevisto nel calcolo delle proprietà"); }
}
Result<Shape> subtract(const Shape& target,const Shape& cutter) noexcept {
    if (!target.impl_ || !cutter.impl_) return Result<Shape>::failure("Forme del taglio non valide");
    try {
        BRepAlgoAPI_Cut operation(target.impl_->shape,cutter.impl_->shape);
        operation.Build();
        if (!operation.IsDone() || operation.Shape().IsNull())
            return Result<Shape>::failure("Sottrazione OpenCascade fallita");
        auto result=Shape(std::make_shared<Shape::Impl>(operation.Shape()));
        if (!mass_properties(result)) return Result<Shape>::failure("La sottrazione ha prodotto un corpo vuoto");
        return Result<Shape>::success(std::move(result));
    } catch (const Standard_Failure& e) { return Result<Shape>::failure(occt_message(e)); }
      catch (const std::exception& e) { return Result<Shape>::failure(e.what()); }
      catch (...) { return Result<Shape>::failure("Errore imprevisto nella sottrazione"); }
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
Result<Tessellation> tessellate(const Shape& shape,double tolerance) noexcept {
    if (!shape.impl_) return Result<Tessellation>::failure("Forma vuota");
    if (!std::isfinite(tolerance) || tolerance<0.001 || tolerance>10.0)
        return Result<Tessellation>::failure("Tolleranza di tassellazione non valida");
    try {
        BRepMesh_IncrementalMesh mesher(shape.impl_->shape,tolerance,false,0.35,true);
        if (!mesher.IsDone()) return Result<Tessellation>::failure("Tassellazione OpenCascade fallita");
        Tessellation result;
        int face_id=0;
        auto point=[](const gp_Pnt& p)->Vec3 { return {p.X(),p.Y(),p.Z()}; };
        for (TopExp_Explorer explorer(shape.impl_->shape,TopAbs_FACE);explorer.More();explorer.Next()) {
            ++face_id;
            const auto face=TopoDS::Face(explorer.Current());
            TopLoc_Location location;
            const auto triangulation=BRep_Tool::Triangulation(face,location);
            if (triangulation.IsNull()) continue;
            for (int i=1;i<=triangulation->NbTriangles();++i) {
                int a=0,b=0,c=0;
                triangulation->Triangle(i).Get(a,b,c);
                if (face.Orientation()==TopAbs_REVERSED) std::swap(b,c);
                const auto pa=point(triangulation->Node(a).Transformed(location.Transformation()));
                const auto pb=point(triangulation->Node(b).Transformed(location.Transformation()));
                const auto pc=point(triangulation->Node(c).Transformed(location.Transformation()));
                const double ux=pb.x-pa.x,uy=pb.y-pa.y,uz=pb.z-pa.z;
                const double vx=pc.x-pa.x,vy=pc.y-pa.y,vz=pc.z-pa.z;
                Vec3 normal{uy*vz-uz*vy,uz*vx-ux*vz,ux*vy-uy*vx};
                const double size=std::hypot(normal.x,normal.y,normal.z);
                if (size<=1.0e-12) continue;
                normal.x/=size; normal.y/=size; normal.z/=size;
                result.triangles.push_back({{{pa,pb,pc}},normal,face_id});
                if (result.triangles.size()>2000000) return Result<Tessellation>::failure("Troppi triangoli");
            }
        }
        for (TopExp_Explorer explorer(shape.impl_->shape,TopAbs_EDGE);explorer.More();explorer.Next()) {
            const auto edge=TopoDS::Edge(explorer.Current());
            BRepAdaptor_Curve curve(edge);
            const double first=curve.FirstParameter(),last=curve.LastParameter();
            if (!std::isfinite(first) || !std::isfinite(last) || last<first) continue;
            const int count=curve.GetType()==GeomAbs_Line?1:48;
            Polyline line;
            for (int i=0;i<=count;++i)
                line.points.push_back(point(curve.Value(first+(last-first)*i/count)));
            result.edges.push_back(std::move(line));
        }
        if (result.triangles.empty()) return Result<Tessellation>::failure("La forma non contiene triangoli");
        return Result<Tessellation>::success(std::move(result));
    } catch (const Standard_Failure& e) { return Result<Tessellation>::failure(occt_message(e)); }
      catch (const std::exception& e) { return Result<Tessellation>::failure(e.what()); }
      catch (...) { return Result<Tessellation>::failure("Errore imprevisto nella tassellazione"); }
}
} // namespace sagomacad::kernel
