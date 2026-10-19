#include "kernel/shape.hpp"
#include "kernel/shape_impl.hpp"
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
#include <BRepLProp_SLProps.hxx>
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
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepPrimAPI_MakeRevol.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <gp_Circ.hxx>
#include <gp_Ax2.hxx>
#include <TopoDS_Wire.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <algorithm>
#include <numbers>

namespace sagomacad::kernel {
namespace {
bool positive(double value) { return std::isfinite(value) && value>0.0 && value<=1.0e9; }
std::string occt_message(const Standard_Failure& failure) {
    const char* text=failure.GetMessageString();
    return text && *text ? std::string(text) : "Operazione OpenCascade fallita";
}
}
Result<Shape> profile_face(const doc::Sketch& s,const std::vector<std::string>& boundary,const std::vector<std::vector<std::string>>& holes) noexcept {
    try {
        if(boundary.empty()||boundary.size()>4096||holes.size()>1024)throw std::runtime_error("Contorno del profilo non valido");
        auto world=[&](double x,double y){if(s.support)return gp_Pnt(s.origin[0]+x*s.u[0]+y*s.v[0],s.origin[1]+x*s.u[1]+y*s.v[1],s.origin[2]+x*s.u[2]+y*s.v[2]);return s.plane=="XY"?gp_Pnt(x,y,0):s.plane=="XZ"?gp_Pnt(x,0,y):gp_Pnt(0,x,y);};
        auto point=[&](const std::string& id){for(const auto& p:s.points)if(p.id==id)return world(p.x,p.y);throw std::runtime_error("Punto del profilo mancante: "+id);};
        const gp_Dir normal=s.support?gp_Dir(gp_Vec(s.u[0],s.u[1],s.u[2]).Crossed(gp_Vec(s.v[0],s.v[1],s.v[2]))):s.plane=="XY"?gp_Dir(0,0,1):s.plane=="XZ"?gp_Dir(0,-1,0):gp_Dir(1,0,0);
        std::vector<NamedShape> names;
        auto wire=[&](const std::vector<std::string>& ids){
            BRepBuilderAPI_MakeWire builder;
            for(const auto& id:ids){
                auto it=std::find_if(s.entities.begin(),s.entities.end(),[&](const auto& e){return e.id==id&&!e.construction;});
                if(it==s.entities.end())throw std::runtime_error("Entità del profilo mancante: "+id);
                const auto& e=*it;
                TopoDS_Edge edge;
                if(e.kind=="line")edge=BRepBuilderAPI_MakeEdge(point(e.points.at(0)),point(e.points.at(1))).Edge();
                else if(e.kind=="circle")edge=BRepBuilderAPI_MakeEdge(gp_Circ(gp_Ax2(point(e.points.at(0)),normal),e.radius)).Edge();
                else if(e.kind=="arc"){
                    auto c=point(e.points.at(0)),a=point(e.points.at(1)),b=point(e.points.at(2));
                    auto local=[&](gp_Pnt p){if(s.support){auto d=gp_Vec(gp_Pnt(s.origin[0],s.origin[1],s.origin[2]),p);return std::pair(d.Dot(gp_Vec(s.u[0],s.u[1],s.u[2])),d.Dot(gp_Vec(s.v[0],s.v[1],s.v[2])));}return s.plane=="XY"?std::pair(p.X(),p.Y()):s.plane=="XZ"?std::pair(p.X(),p.Z()):std::pair(p.Y(),p.Z());};
                    auto [cx,cy]=local(c);auto [ax,ay]=local(a);auto [bx,by]=local(b);
                    double u=std::atan2(ay-cy,ax-cx),v=std::atan2(by-cy,bx-cx);while(v<=u)v+=2*std::numbers::pi;
                    GC_MakeArcOfCircle arc(a,world(cx+e.radius*std::cos((u+v)/2),cy+e.radius*std::sin((u+v)/2)),b);
                    if(!arc.IsDone())throw std::runtime_error("Arco del profilo non valido");
                    edge=BRepBuilderAPI_MakeEdge(arc.Value()).Edge();
                }else throw std::runtime_error("Entità non ammessa nel profilo");
                builder.Add(edge);names.push_back({"entity("+id+")",edge});
                if(!builder.IsDone())throw std::runtime_error("Contorno del profilo non connesso");
            }
            if(!builder.IsDone()||!builder.Wire().Closed())throw std::runtime_error("Il profilo non è chiuso");
            // MakeWire may replace vertices and therefore edge TShapes. Retain
            // sketch identity by the shared OCCT curve and exact parameter range.
            TopTools_IndexedMapOfShape wire_edges;TopExp::MapShapes(builder.Wire(),TopAbs_EDGE,wire_edges);
            for(auto& named:names)if(!wire_edges.Contains(named.shape)){
                double a=0,b=0;TopLoc_Location location;auto curve=BRep_Tool::Curve(TopoDS::Edge(named.shape),location,a,b);
                if(curve.IsNull())continue;
                for(int i=1;i<=wire_edges.Extent();++i){double u=0,v=0;TopLoc_Location other;auto candidate=BRep_Tool::Curve(TopoDS::Edge(wire_edges(i)),other,u,v);
                    if(candidate==curve&&location.IsEqual(other)&&std::abs(a-u)<1e-12&&std::abs(b-v)<1e-12){named.shape=wire_edges(i);break;}}
            }
            return builder.Wire();
        };
        BRepBuilderAPI_MakeFace outer(wire(boundary),true);if(!outer.IsDone())throw std::runtime_error("Faccia del profilo non valida");
        TopoDS_Shape face=outer.Face();
        for(const auto& h:holes){
            BRepBuilderAPI_MakeFace hole(wire(h),true);if(!hole.IsDone())throw std::runtime_error("Foro del profilo non valido");
            auto input=ShapeAccess::make(face,names),cutter=ShapeAccess::make(hole.Face(),names);
            BRepAlgoAPI_Cut cut(face,hole.Face());cut.Build();if(!cut.IsDone())throw std::runtime_error("Foro del profilo fallito");
            auto raw=ShapeAccess::make(cut.Shape());propagate(cut,*ShapeAccess::get(input),*ShapeAccess::get(raw));propagate(cut,*ShapeAccess::get(cutter),*ShapeAccess::get(raw));
            const auto cleaned=unify_same_domain(raw);face=ShapeAccess::get(cleaned)->shape;names=ShapeAccess::get(cleaned)->names;
        }
        return Result<Shape>::success(ShapeAccess::make(face,std::move(names)));
    }catch(const Standard_Failure& e){return Result<Shape>::failure(occt_message(e));}
    catch(const std::exception& e){return Result<Shape>::failure(e.what());}catch(...){return Result<Shape>::failure("Creazione profilo fallita");}
}
Result<Shape> extrude(const Shape& face,Vec3 v) noexcept {
    if(!face.impl_||!std::isfinite(v.x)||!std::isfinite(v.y)||!std::isfinite(v.z)||!positive(std::hypot(v.x,v.y,v.z)))return Result<Shape>::failure("Vettore estrusione non valido");
    try{
        BRepPrimAPI_MakePrism builder(face.impl_->shape,gp_Vec(v.x,v.y,v.z));builder.Build();if(!builder.IsDone())throw std::runtime_error("Estrusione fallita");
        auto shape=ShapeAccess::make(builder.Shape());
        for(TopExp_Explorer f(builder.FirstShape(),TopAbs_FACE);f.More();f.Next())shape.impl_->names.push_back({"cap_start",f.Current()});
        for(TopExp_Explorer f(builder.LastShape(),TopAbs_FACE);f.More();f.Next())shape.impl_->names.push_back({"cap_end",f.Current()});
        TopTools_IndexedMapOfShape inputs;TopExp::MapShapes(face.impl_->shape,inputs);
        for(const auto& n:face.impl_->names)if(n.shape.ShapeType()==TopAbs_EDGE&&inputs.Contains(n.shape))for(const auto& generated:builder.Generated(n.shape))
            if(generated.ShapeType()==TopAbs_FACE)shape.impl_->names.push_back({"side("+n.name+")",generated});
        auto mass=mass_properties(shape);if(!mass)throw std::runtime_error(mass.error);return Result<Shape>::success(shape);
    }
    catch(const Standard_Failure& e){return Result<Shape>::failure(occt_message(e));}catch(const std::exception& e){return Result<Shape>::failure(e.what());}catch(...){return Result<Shape>::failure("Estrusione fallita");}
}
Result<Shape> revolve(const Shape& face,Vec3 origin,Vec3 direction,double radians) noexcept {
    if(!face.impl_||!std::isfinite(radians)||radians<=1e-8||radians>2*std::numbers::pi+1e-8||
       !std::isfinite(origin.x)||!std::isfinite(origin.y)||!std::isfinite(origin.z)||
       !std::isfinite(direction.x)||!std::isfinite(direction.y)||!std::isfinite(direction.z)||std::hypot(direction.x,direction.y,direction.z)<1e-8)
        return Result<Shape>::failure("Asse o angolo della rivoluzione non valido");
    try{
        BRepPrimAPI_MakeRevol builder(face.impl_->shape,gp_Ax1(gp_Pnt(origin.x,origin.y,origin.z),gp_Dir(direction.x,direction.y,direction.z)),radians,true);
        builder.Build();if(!builder.IsDone())throw std::runtime_error("Rivoluzione fallita");
        auto shape=Shape(std::make_shared<Shape::Impl>(builder.Shape()));
        TopTools_IndexedMapOfShape inputs,present;TopExp::MapShapes(face.impl_->shape,inputs);TopExp::MapShapes(shape.impl_->shape,present);
        for(const auto& named:face.impl_->names)if(named.shape.ShapeType()==TopAbs_EDGE&&inputs.Contains(named.shape))for(const auto& generated:builder.Generated(named.shape))
            if(generated.ShapeType()==TopAbs_FACE&&present.Contains(generated))shape.impl_->names.push_back({"side("+named.name+")",generated});
        auto cap=[&](const TopoDS_Shape& end,const char* name){if(end.IsNull())return;for(TopExp_Explorer f(end,TopAbs_FACE);f.More();f.Next())if(present.Contains(f.Current()))shape.impl_->names.push_back({name,f.Current()});};
        cap(builder.FirstShape(),"cap_start");cap(builder.LastShape(),"cap_end");
        auto mass=mass_properties(shape);if(!mass)throw std::runtime_error(mass.error);return Result<Shape>::success(shape);
    }catch(const Standard_Failure& e){return Result<Shape>::failure(occt_message(e));}catch(const std::exception& e){return Result<Shape>::failure(e.what());}catch(...){return Result<Shape>::failure("Rivoluzione fallita");}
}
Result<Shape> boolean_operation(const Shape& a,const Shape& b,const std::string& op) noexcept {
    if(op=="cut"||op=="subtract")return subtract(a,b);
    if(!a.impl_||!b.impl_)return Result<Shape>::failure("Corpo o utensile assente");
    try{
        auto output=ShapeAccess::make({});auto& named=*ShapeAccess::get(output);
        if(op=="join"){BRepAlgoAPI_Fuse operation(a.impl_->shape,b.impl_->shape);operation.Build();if(!operation.IsDone())throw std::runtime_error("Unione fallita");named.shape=operation.Shape();propagate(operation,*a.impl_,named);propagate(operation,*b.impl_,named);}
        else if(op=="intersect"){BRepAlgoAPI_Common operation(a.impl_->shape,b.impl_->shape);operation.Build();if(!operation.IsDone())throw std::runtime_error("Intersezione fallita");named.shape=operation.Shape();propagate(operation,*a.impl_,named);propagate(operation,*b.impl_,named);}
        else throw std::runtime_error("Operazione sconosciuta: "+op);
        auto mass=mass_properties(output);if(!mass)throw std::runtime_error("L'operazione ha prodotto un corpo vuoto");return Result<Shape>::success(unify_same_domain(output));
    }catch(const Standard_Failure& e){return Result<Shape>::failure(occt_message(e));}catch(const std::exception& e){return Result<Shape>::failure(e.what());}catch(...){return Result<Shape>::failure("Operazione booleana fallita");}
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
        propagate(operation,*target.impl_,*result.impl_);propagate(operation,*cutter.impl_,*result.impl_);
        if (!mass_properties(result)) return Result<Shape>::failure("La sottrazione ha prodotto un corpo vuoto");
        return Result<Shape>::success(unify_same_domain(result));
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
        auto result=Shape(std::make_shared<Shape::Impl>(moved));propagate(builder,*shape.impl_,*result.impl_);
        return Result<Shape>::success(result);
    } catch (const Standard_Failure& e) { return Result<Shape>::failure(occt_message(e)); }
      catch (const std::exception& e) { return Result<Shape>::failure(e.what()); }
      catch (...) { return Result<Shape>::failure("Errore imprevisto nella traslazione"); }
}
Result<Tessellation> tessellate(const Shape& shape,double tolerance) noexcept {
    if (!shape.impl_) return Result<Tessellation>::failure("Forma vuota");
    if (!std::isfinite(tolerance) || tolerance<0.001 || tolerance>10.0)
        return Result<Tessellation>::failure("Tolleranza di tassellazione non valida");
    try {
        BRepMesh_IncrementalMesh mesher(shape.impl_->shape,tolerance,false,0.10,true);
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
            BRepAdaptor_Surface surface(face);
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
                Triangle triangle{{{pa,pb,pc}},normal,face_id};
                const std::array<int,3> indices{a,b,c};
                for(int vertex=0;vertex<3;++vertex){
                    auto smooth=normal;
                    if(triangulation->HasUVNodes()){
                        const auto uv=triangulation->UVNode(indices[vertex]);
                        BRepLProp_SLProps properties(surface,uv.X(),uv.Y(),1,1e-9);
                        if(properties.IsNormalDefined()){
                            auto n=properties.Normal();if(face.Orientation()==TopAbs_REVERSED)n.Reverse();
                            smooth={n.X(),n.Y(),n.Z()};
                        }
                    }
                    triangle.normals[vertex]=smooth;
                }
                result.triangles.push_back(triangle);
                if (result.triangles.size()>2000000) return Result<Tessellation>::failure("Troppi triangoli");
            }
        }
        TopTools_IndexedMapOfShape edge_map;
        TopExp::MapShapes(shape.impl_->shape,TopAbs_EDGE,edge_map);
        for (int edge_id=1;edge_id<=edge_map.Extent();++edge_id) {
            const auto edge=TopoDS::Edge(edge_map(edge_id));
            BRepAdaptor_Curve curve(edge);
            const double first=curve.FirstParameter(),last=curve.LastParameter();
            if (!std::isfinite(first) || !std::isfinite(last) || last<first) continue;
            const int count=curve.GetType()==GeomAbs_Line?1:48;
            Polyline line;
            line.edge_id=edge_id;
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
