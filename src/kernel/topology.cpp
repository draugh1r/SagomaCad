#include "kernel/topology.hpp"
#include "kernel/shape_impl.hpp"
#include <BRepAdaptor_Surface.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepGProp.hxx>
#include <BRep_Tool.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepFilletAPI_MakeChamfer.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <GProp_GProps.hxx>
#include <Standard_Failure.hxx>
#include <TopoDS.hxx>
#include <cmath>
#include <map>
#include <sstream>
#include <cstdint>

namespace sagomacad::kernel {
namespace {
Vec3 vec(const gp_XYZ& p){return {p.X(),p.Y(),p.Z()};}
std::array<double,3> array(Vec3 p){return {p.x,p.y,p.z};}
TopAbs_ShapeEnum shape_type(const std::string& k){return k=="face"?TopAbs_FACE:k=="edge"?TopAbs_EDGE:TopAbs_VERTEX;}
doc::GeometrySignature signature(const TopoDS_Shape& s){
    doc::GeometrySignature r;GProp_GProps props;Vec3 direction{};
    if(s.ShapeType()==TopAbs_FACE){
        r.kind="face";auto face=TopoDS::Face(s);BRepAdaptor_Surface surface(face);BRepGProp::SurfaceProperties(face,props);
        if(surface.GetType()==GeomAbs_Plane){r.type="plane";direction=vec(surface.Plane().Axis().Direction().XYZ());if(face.Orientation()==TopAbs_REVERSED){direction.x=-direction.x;direction.y=-direction.y;direction.z=-direction.z;}}
        else if(surface.GetType()==GeomAbs_Cylinder){r.type="cylinder";direction=vec(surface.Cylinder().Axis().Direction().XYZ());}
        else if(surface.GetType()==GeomAbs_Sphere)r.type="sphere";else r.type="surface:"+std::to_string(int(surface.GetType()));
    }else if(s.ShapeType()==TopAbs_EDGE){
        r.kind="edge";auto edge=TopoDS::Edge(s);BRepAdaptor_Curve curve(edge);BRepGProp::LinearProperties(edge,props);
        if(curve.GetType()==GeomAbs_Line){r.type="line";direction=vec(curve.Line().Direction().XYZ());
            // Canonical orientation independent of the orientation of the wire.
            if(direction.x< -1e-9||(std::abs(direction.x)<1e-9&&direction.y< -1e-9)||(std::abs(direction.x)+std::abs(direction.y)<1e-9&&direction.z<0)){direction.x=-direction.x;direction.y=-direction.y;direction.z=-direction.z;}}
        else if(curve.GetType()==GeomAbs_Circle)r.type="circle";else r.type="curve:"+std::to_string(int(curve.GetType()));
    }else{r.kind="vertex";r.type="point";r.center=array(vec(BRep_Tool::Pnt(TopoDS::Vertex(s)).XYZ()));return r;}
    r.center=array(vec(props.CentreOfMass().XYZ()));r.direction=array(direction);r.measure=props.Mass();return r;
}
std::string axis_name(const doc::GeometrySignature& s){
    if(s.type!="plane")return s.type;
    for(int i=0;i<3;++i)if(std::abs(s.direction[i])>1-1e-8)return std::string(1,"xyz"[i])+(s.direction[i]>0?"_max":"_min");
    std::ostringstream out;out<<"plane("<<s.direction[0]<<","<<s.direction[1]<<","<<s.direction[2]<<")";return out.str();
}
std::string existing(const auto& impl,const TopoDS_Shape& shape){std::string fallback;for(const auto& n:impl.names)if(n.shape.IsSame(shape)){if(n.name.size()<=48)return n.name;if(fallback.empty())fallback=n.name;}return fallback;}
bool close(double a,double b){return std::isfinite(a)&&std::isfinite(b)&&std::abs(a-b)<=1e-7*std::max({1.,std::abs(a),std::abs(b)});}
bool same(const doc::GeometrySignature& a,const doc::GeometrySignature& b){
    if(a.kind!=b.kind||a.type!=b.type||!close(a.measure,b.measure))return false;
    for(int i=0;i<3;++i)if(!close(a.center[i],b.center[i])||!close(a.direction[i],b.direction[i]))return false;return true;
}
}
Result<Shape> name_topology(const Shape& source,const std::string& origin)noexcept{
    try{
        const auto& p=ShapeAccess::get(source);if(!p||origin.empty()||origin.size()>256)throw std::runtime_error("Origine topologica non valida");
        auto result=ShapeAccess::make(p->shape,p->names,p->deleted);auto& out=*ShapeAccess::get(result);
        // Existing semantic names (caps/sides/history) acquire their feature origin.
        for(auto& n:out.names)if(n.name.find('/')==std::string::npos||n.name.rfind("side(",0)==0)n.name=origin+"/"+n.name;
        for(auto kind:{TopAbs_FACE,TopAbs_EDGE,TopAbs_VERTEX}){
            TopTools_IndexedMapOfShape map;TopExp::MapShapes(out.shape,kind,map);
            std::map<std::string,int> occurrences;
            for(int i=1;i<=map.Extent();++i){auto shape=map(i);if(!existing(out,shape).empty())continue;
                std::string label;
                if(kind==TopAbs_FACE)label=origin+"/"+axis_name(signature(shape));
                else{
                    std::set<std::string> adjacent;
                    for(const auto& named:out.names)if(named.shape.ShapeType()==TopAbs_FACE){
                        for(TopExp_Explorer e(named.shape,kind);e.More();e.Next())if(e.Current().IsSame(shape)){adjacent.insert(named.name);break;}}
                    label=origin+(kind==TopAbs_EDGE?"/edge(":"/vertex(");
                    for(const auto& a:adjacent){if(label.back()!='(')label+=",";label+=a;}label+=")";
                }
                // Collisions remain aliases and are rejected on resolution. IDs
                // are never appended to turn an ambiguous semantic name into a guess.
                out.names.push_back({label,shape});
            }
        }
        const auto verbose=out.names;
        for(const auto& named:verbose)if(named.name.size()>48){std::uint64_t hash=14695981039346656037ULL;for(unsigned char c:named.name){hash^=c;hash*=1099511628211ULL;}std::ostringstream suffix;suffix<<std::hex<<hash;auto slash=named.name.find('/');auto source=named.name.substr(0,slash);auto role=named.shape.ShapeType()==TopAbs_FACE?"f":named.shape.ShapeType()==TopAbs_EDGE?"e":"v";auto alias=source+"/"+role+"-"+suffix.str();if(std::none_of(out.names.begin(),out.names.end(),[&](const auto& n){return n.name==alias&&n.shape.IsSame(named.shape);}))out.names.push_back({alias,named.shape});}
        return Result<Shape>::success(result);
    }catch(const Standard_Failure& e){return Result<Shape>::failure(e.GetMessageString()?e.GetMessageString():"Nomi topologici falliti");}
    catch(const std::exception& e){return Result<Shape>::failure(e.what());}catch(...){return Result<Shape>::failure("Nomi topologici falliti");}
}
Result<std::vector<TopologyEntity>> topology(const Shape& source)noexcept{
    try{
        const auto& p=ShapeAccess::get(source);if(!p)throw std::runtime_error("Forma assente");
        std::vector<TopologyEntity> out;TopTools_IndexedMapOfShape edges;TopExp::MapShapes(p->shape,TopAbs_EDGE,edges);
        for(auto kind:{TopAbs_FACE,TopAbs_EDGE,TopAbs_VERTEX}){
            TopTools_IndexedMapOfShape map;TopExp::MapShapes(p->shape,kind,map);
            for(int i=1;i<=map.Extent();++i){TopologyEntity r;r.id=i;r.name=existing(*p,map(i));r.signature=signature(map(i));
                if(kind==TopAbs_FACE)for(TopExp_Explorer e(map(i),TopAbs_EDGE);e.More();e.Next()){int id=edges.FindIndex(e.Current());if(std::find(r.edges.begin(),r.edges.end(),id)==r.edges.end())r.edges.push_back(id);}
                out.push_back(std::move(r));
            }
        }return Result<std::vector<TopologyEntity>>::success(std::move(out));
    }catch(const Standard_Failure& e){return Result<std::vector<TopologyEntity>>::failure(e.GetMessageString()?e.GetMessageString():"Query topologica fallita");}
    catch(const std::exception& e){return Result<std::vector<TopologyEntity>>::failure(e.what());}catch(...){return Result<std::vector<TopologyEntity>>::failure("Query topologica fallita");}
}
Result<TopologyEntity> resolve(const Shape& source,const doc::TopologyRef& ref,const std::string& kind)noexcept{
    try{
        const auto& p=ShapeAccess::get(source);if(!p)throw std::runtime_error("Corpo del riferimento assente: "+ref.body);
        if(kind!="face"&&kind!="edge"&&kind!="vertex")throw std::runtime_error("Tipo di riferimento non valido");
        TopTools_IndexedMapOfShape map;TopExp::MapShapes(p->shape,shape_type(kind),map);std::set<int> matches;
        for(const auto& n:p->names)if(n.name==ref.name&&n.shape.ShapeType()==shape_type(kind)){int i=map.FindIndex(n.shape);if(i)matches.insert(i);}
        if(matches.size()>1)throw std::runtime_error("Riferimento ambiguo dopo divisione: "+ref.name);
        if(matches.empty()&&p->deleted.contains(ref.name))throw std::runtime_error("Entità eliminata: "+ref.name);
        if(matches.empty()){auto slash=ref.name.find('/');if(slash!=std::string::npos){auto origin=ref.name.substr(0,slash+1);if(std::none_of(p->names.begin(),p->names.end(),[&](const auto& n){return n.name.rfind(origin,0)==0;}))throw std::runtime_error("Origine del riferimento non disponibile: "+ref.name);}}
        auto rows=topology(source);if(!rows)throw std::runtime_error(rows.error);
        if(matches.empty())for(const auto& r:*rows.value)if(r.signature.kind==kind&&same(r.signature,ref.signature))matches.insert(r.id);
        if(matches.size()!=1)throw std::runtime_error((matches.empty()?"Riferimento non risolto: ":"Firma geometrica ambigua: ")+ref.name);
        for(const auto& r:*rows.value)if(r.signature.kind==kind&&r.id==*matches.begin())return Result<TopologyEntity>::success(r);
        throw std::runtime_error("Riferimento non risolto: "+ref.name);
    }catch(const Standard_Failure& e){return Result<TopologyEntity>::failure(e.GetMessageString()?e.GetMessageString():"Riferimento topologico fallito");}
    catch(const std::exception& e){return Result<TopologyEntity>::failure(e.what());}catch(...){return Result<TopologyEntity>::failure("Riferimento topologico fallito");}
}
Result<Shape> face_shape(const Shape& source,int id)noexcept{
    try{const auto& p=ShapeAccess::get(source);if(!p)throw std::runtime_error("Corpo assente");TopTools_IndexedMapOfShape map;TopExp::MapShapes(p->shape,TopAbs_FACE,map);
        if(id<1||id>map.Extent())throw std::runtime_error("ID faccia fuori intervallo");
        return Result<Shape>::success(ShapeAccess::make(map(id),p->names,p->deleted));
    }catch(const Standard_Failure& e){return Result<Shape>::failure(e.GetMessageString()?e.GetMessageString():"Faccia non disponibile");}catch(const std::exception& e){return Result<Shape>::failure(e.what());}catch(...){return Result<Shape>::failure("Faccia non disponibile");}
}
Result<FaceFrame> face_frame(const Shape& source,int id)noexcept{
    try{auto face=face_shape(source,id);if(!face)throw std::runtime_error(face.error);auto s=TopoDS::Face(ShapeAccess::get(*face.value)->shape);BRepAdaptor_Surface a(s);
        if(a.GetType()!=GeomAbs_Plane)throw std::runtime_error("È richiesta una faccia piana");auto plane=a.Plane();auto normal=plane.Axis().Direction(),u=plane.Position().XDirection();
        if(s.Orientation()==TopAbs_REVERSED)normal.Reverse();gp_Dir v(normal.Crossed(u));
        return Result<FaceFrame>::success({vec(plane.Location().XYZ()),vec(u.XYZ()),vec(v.XYZ()),vec(normal.XYZ())});
    }catch(const Standard_Failure& e){return Result<FaceFrame>::failure(e.GetMessageString()?e.GetMessageString():"Piano della faccia non disponibile");}catch(const std::exception& e){return Result<FaceFrame>::failure(e.what());}catch(...){return Result<FaceFrame>::failure("Piano della faccia non disponibile");}
}
namespace {
template<class Builder> Result<Shape> finish_edges(Builder& b,const Shape& source,const std::vector<int>& ids,double amount,const std::string& origin){
    const auto& p=*ShapeAccess::get(source);TopTools_IndexedMapOfShape edges;TopExp::MapShapes(p.shape,TopAbs_EDGE,edges);
    std::set<int> seen;
    for(int id:ids){if(id<1||id>edges.Extent()||!seen.insert(id).second)throw std::runtime_error("Spigolo non valido o duplicato: "+std::to_string(id));b.Add(amount,TopoDS::Edge(edges(id)));}
    b.Build();if(!b.IsDone()||b.Shape().IsNull()||!BRepCheck_Analyzer(b.Shape()).IsValid())throw std::runtime_error("OCCT non può costruire la feature");
    auto out=ShapeAccess::make(b.Shape());auto& result=*ShapeAccess::get(out);propagate(b,p,result);
    for(const auto& n:p.names)if(n.shape.ShapeType()==TopAbs_EDGE)for(const auto& generated:b.Generated(n.shape))
        if(generated.ShapeType()==TopAbs_FACE)result.names.push_back({origin+"/blend("+n.name+")",generated});
    auto mass=mass_properties(out);if(!mass)throw std::runtime_error(mass.error);return name_topology(out,origin);
}
Result<Shape> dress(const Shape& source,const std::vector<int>& ids,double amount,const std::string& origin,bool round)noexcept{
    std::string context=round?"Raccordo":"Smusso";
    for(int id:ids)context+=" spigolo "+std::to_string(id);
    try{const auto& p=ShapeAccess::get(source);if(!p||ids.empty()||ids.size()>4096||!std::isfinite(amount)||amount<=0||amount>1e7)throw std::runtime_error("Spigoli o misura non validi");
        if(round){BRepFilletAPI_MakeFillet b(p->shape);return finish_edges(b,source,ids,amount,origin);}BRepFilletAPI_MakeChamfer b(p->shape);return finish_edges(b,source,ids,amount,origin);
    }catch(const Standard_Failure& e){return Result<Shape>::failure(context+": "+(e.GetMessageString()?e.GetMessageString():"operazione OCCT fallita"));}catch(const std::exception& e){return Result<Shape>::failure(context+": "+e.what());}catch(...){return Result<Shape>::failure(context+": operazione fallita");}
}
}
Result<Shape> fillet(const Shape& s,const std::vector<int>& ids,double r,const std::string& origin)noexcept{return dress(s,ids,r,origin,true);}
Result<Shape> chamfer(const Shape& s,const std::vector<int>& ids,double d,const std::string& origin)noexcept{return dress(s,ids,d,origin,false);}
}
