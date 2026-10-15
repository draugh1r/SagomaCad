#include "engine/command.hpp"
#include "engine/cmds/validation.hpp"
#include "solver/sketch_solver.hpp"
#include "doc/expression.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace sagomacad::engine::cmds {
namespace {
using J=nlohmann::json;
std::string unique(const doc::Sketch& s,std::string prefix){auto j=doc::sketch_json(s);for(size_t n=1;;++n){auto id=prefix+std::to_string(n);bool used=false;for(const char* key:{"points","entities","constraints","dimensions"})for(const auto& e:j[key])if(e["id"]==id)used=true;if(!used)return id;}}
double coordinate(const J& p,const char* key,double fallback=0){if(!p.contains(key))return fallback;if(!p[key].is_number())throw std::runtime_error(std::string(key)+": è richiesto un numero");double n=p[key].get<double>();if(!std::isfinite(n)||std::abs(n)>1e6)throw std::runtime_error("sketch.range: coordinata fuori intervallo");return n;}
double expression(const doc::Document& d,const std::string& text,bool angle=false){doc::Quantity q;std::string error;if(!doc::evaluate_expression(d,text,q,error))throw std::runtime_error(error);if(q.dimension!=(angle?doc::Dimension::Angle:doc::Dimension::Length)&&q.dimension!=doc::Dimension::Scalar)throw std::runtime_error("Unità incompatibili");if(!std::isfinite(q.value)||std::abs(q.value)>1e6)throw std::runtime_error("Quota fuori intervallo");return angle&&q.dimension==doc::Dimension::Scalar?q.value*std::numbers::pi/180:q.value;}
std::string point(doc::Sketch& s,double x,double y){auto id=unique(s,"p");s.points.push_back({id,x,y});return id;}
std::string entity(doc::Sketch& s,std::string kind,std::vector<std::string> points,double radius=1,bool construction=false){auto id=unique(s,"e");s.entities.push_back({id,kind,points,radius,construction});return id;}
std::string constraint(doc::Sketch& s,std::string kind,std::vector<std::string> refs,std::vector<double> values={}){auto id=unique(s,"c");s.constraints.push_back({id,kind,refs,{},values});return id;}
std::string dimension(doc::Sketch& s,std::string kind,std::vector<std::string> refs,std::string expr){auto id=unique(s,"d");s.dimensions.push_back({id,kind,refs,expr,{}});return id;}
void connect(doc::Sketch& s,const std::string& id,double x,double y){
    for(const auto& p:s.points)if(p.id!=id&&std::hypot(p.x-x,p.y-y)<1e-7){constraint(s,"coincident",{id,p.id});break;}
}
doc::Feature& feature(Session& s,const J& p){auto id=p.contains("sketch")?string_arg(p,"sketch",true,128):s.active_sketch;if(id.empty())throw std::runtime_error("sketch.not_active: crea o apri uno schizzo");auto it=s.document.features.find(id);if(it==s.document.features.end()||it->second.type!="sketch")throw std::runtime_error("sketch.not_found: schizzo inesistente: "+id);return it->second;}
CommandResult store(Session& s,doc::Feature& f,const doc::Sketch& sketch,J extra=J::object()){
    auto result=solver::solve(s.document,sketch);if(result.state=="error")return CommandResult::failure(result.error);
    f.data=doc::sketch_json(result.sketch);extra["sketch"]=f.id;extra["solver"]=solver::inspect(result);return {true,extra,{}};
}
CommandSpec spec(std::string id,std::string label,std::string help,std::function<CommandResult(Session&,const J&)> run,bool journal=true,std::optional<std::string> shortcut={}){
    return {id,label,{"Schizzo","Crea"},shortcut,help,{},std::move(run),journal};
}
}
std::vector<CommandSpec> sketch_specs(){
    std::vector<CommandSpec> specs;
    specs.push_back(spec("sketch.create","Crea schizzo","{plane: XY|XZ|YZ, name?: string}",[](Session& s,const J& p){
        only_keys(p,{"plane","name"});doc::Sketch sketch;sketch.plane=string_arg(p,"plane",true,2);sketch=doc::read_sketch(doc::sketch_json(sketch));
        size_t n=s.document.timeline.size()+1;std::string id;do{id="sketch-"+std::to_string(n++);}while(s.document.features.contains(id));
        doc::Feature f;f.id=id;f.name=p.contains("name")?string_arg(p,"name",true,128):"Schizzo "+std::to_string(n-1);f.type="sketch";f.data=doc::sketch_json(sketch);
        s.document.features[id]=f;s.document.timeline.push_back(id);s.document.rollback=s.document.timeline.size();s.active_sketch=id;return CommandResult{true,{{"sketch",id}},{}};
    }));
    specs.push_back(spec("sketch.finish","Fine schizzo","{sketch?: id}",[](Session& s,const J& p){only_keys(p,{"sketch"});auto& f=feature(s,p);auto id=f.id;s.active_sketch.clear();s.document.rollback=s.document.timeline.size();return CommandResult{true,{{"sketch",id}},{}};},false));
    specs.push_back(spec("sketch.edit","Modifica schizzo","{sketch: id}",[](Session& s,const J& p){only_keys(p,{"sketch"});auto& f=feature(s,p);s.active_sketch=f.id;return CommandResult{true,{{"sketch",f.id}},{}};},false));
    specs.push_back(spec("sketch.inspect","Ispeziona schizzo","{sketch?: id}",[](Session& s,const J& p){only_keys(p,{"sketch"});auto& f=feature(s,p);auto j=solver::inspect(solver::solve(s.document,doc::read_sketch(f.data)));j["sketch"]=f.id;return CommandResult{true,j,{}};},false));
    specs.push_back(spec("sketch.rect","Rettangolo","{sketch?: id, x?: mm, y?: mm, width: expression, height: expression, centered?: bool, anchor?: bool}",[](Session& s,const J& p){
        only_keys(p,{"sketch","x","y","width","height","centered","anchor","construction","flip_x","flip_y"});auto& f=feature(s,p);auto t=doc::read_sketch(f.data);
        const auto w=string_arg(p,"width"),h=string_arg(p,"height");double width=expression(s.document,w),height=expression(s.document,h);if(width<=0||height<=0)throw std::runtime_error("Larghezza e altezza devono essere positive");
        bool centered=p.value("centered",false),anchor=p.value("anchor",true);double x=coordinate(p,"x"),y=coordinate(p,"y");
        if(centered&&(x!=0||y!=0))throw std::runtime_error("Il rettangolo centrato usa l'origine: ometti x e y");
        const double dx=width*(p.value("flip_x",false)?-1:1),dy=height*(p.value("flip_y",false)?-1:1);
        if(centered){x=-dx/2;y=-dy/2;}std::vector<std::string> ps={point(t,x,y),point(t,x+dx,y),point(t,x+dx,y+dy),point(t,x,y+dy)};std::vector<std::string> es;
        for(int i=0;i<4;++i){es.push_back(entity(t,"line",{ps[i],ps[(i+1)%4]},1,p.value("construction",false)));constraint(t,i%2?"vertical":"horizontal",{es.back()});}
        if(anchor){if(centered)constraint(t,"centered",{ps[0],ps[2]});else constraint(t,"fix",{ps[0]},{x,y});}
        dimension(t,"length",{es[0]},w);dimension(t,"length",{es[1]},h);return store(s,f,t,{{"entities",es},{"points",ps}});
    },true,"R"));
    specs.push_back(spec("sketch.line","Linea","{sketch?: id, x1,y1,x2,y2: mm, length?: expression, angle?: expression, anchor?: bool}",[](Session& s,const J& p){
        only_keys(p,{"sketch","x1","y1","x2","y2","length","angle","anchor","construction"});auto& f=feature(s,p);auto t=doc::read_sketch(f.data);
        for(const char* key:{"x1","y1","x2","y2"})if(!p.contains(key))throw std::runtime_error(std::string("Manca ")+key);
        double x=coordinate(p,"x1"),y=coordinate(p,"y1"),dx=coordinate(p,"x2")-x,dy=coordinate(p,"y2")-y;
        if(p.contains("length")) {
            double length=expression(s.document,string_arg(p,"length"));
            if(length<=0)throw std::runtime_error("La lunghezza deve essere positiva");
            double angle=p.contains("angle")?expression(s.document,string_arg(p,"angle"),true):std::atan2(dy,dx);
            dx=length*std::cos(angle);dy=length*std::sin(angle);
        }
        if(std::hypot(dx,dy)<1e-8)throw std::runtime_error("La linea deve avere lunghezza positiva");
        auto a=point(t,x,y),b=point(t,x+dx,y+dy),id=entity(t,"line",{a,b},1,p.value("construction",false));
        connect(t,a,x,y);connect(t,b,x+dx,y+dy);
        if(p.value("anchor",false))constraint(t,"fix",{a},{x,y});
        if(p.contains("angle"))dimension(t,"angle",{id},string_arg(p,"angle"));
        else if(std::abs(dy)<1e-6)constraint(t,"horizontal",{id});else if(std::abs(dx)<1e-6)constraint(t,"vertical",{id});
        if(p.contains("length"))dimension(t,"length",{id},string_arg(p,"length"));
        return store(s,f,t,{{"entity",id},{"points",{a,b}}});
    },true,"L"));
    specs.push_back(spec("sketch.circle","Cerchio","{sketch?: id, x?: mm, y?: mm, diameter: expression, anchor?: bool}",[](Session& s,const J& p){
        only_keys(p,{"sketch","x","y","diameter","anchor","construction"});auto& f=feature(s,p);auto t=doc::read_sketch(f.data);
        double x=coordinate(p,"x"),y=coordinate(p,"y");auto expr=string_arg(p,"diameter");double radius=expression(s.document,expr)/2;if(radius<=0)throw std::runtime_error("Il diametro deve essere positivo");
        auto center=point(t,x,y),id=entity(t,"circle",{center},radius,p.value("construction",false));if(p.value("anchor",true))constraint(t,"fix",{center},{x,y});
        dimension(t,"diameter",{id},expr);return store(s,f,t,{{"entity",id},{"center",center}});
    },true,"C"));
    specs.push_back(spec("sketch.arc","Arco","{sketch?: id, x?: mm, y?: mm, radius: expression, start_angle: expression, end_angle: expression, anchor?: bool}",[](Session& s,const J& p){
        only_keys(p,{"sketch","x","y","radius","start_angle","end_angle","anchor","construction"});auto& f=feature(s,p);auto t=doc::read_sketch(f.data);double x=coordinate(p,"x"),y=coordinate(p,"y");
        auto re=string_arg(p,"radius"),a=string_arg(p,"start_angle"),b=string_arg(p,"end_angle");double r=expression(s.document,re),u=expression(s.document,a,true),v=expression(s.document,b,true);if(r<=0||std::abs(std::remainder(u-v,2*std::numbers::pi))<1e-8)throw std::runtime_error("Arco degenere");
        auto center=point(t,x,y),start=point(t,x+r*std::cos(u),y+r*std::sin(u)),end=point(t,x+r*std::cos(v),y+r*std::sin(v));auto id=entity(t,"arc",{center,start,end},r,p.value("construction",false));
        if(p.value("anchor",false))constraint(t,"fix",{center},{x,y});dimension(t,"radius",{id},re);
        dimension(t,"angle",{center,start},a);dimension(t,"angle",{center,end},b);
        return store(s,f,t,{{"entity",id},{"points",{center,start,end}}});
    }));
    specs.push_back(spec("sketch.constraint.add","Vincolo","{sketch?: id, kind: coincident|horizontal|vertical|parallel|perpendicular|tangent|equal|midpoint|fix, refs: [id,...]}",[](Session& s,const J& p){
        only_keys(p,{"sketch","kind","refs"});auto& f=feature(s,p);auto t=doc::read_sketch(f.data);auto kind=string_arg(p,"kind");const std::vector<std::string> kinds={"coincident","horizontal","vertical","parallel","perpendicular","tangent","equal","midpoint","fix"};
        if(std::find(kinds.begin(),kinds.end(),kind)==kinds.end())throw std::runtime_error("Tipo di vincolo sconosciuto");
        auto refs=p.at("refs").get<std::vector<std::string>>();if(refs.empty()||refs.size()>2)throw std::runtime_error("Sono richiesti uno o due riferimenti");
        std::vector<double> values;if(kind=="fix"){
            auto solved=solver::solve(s.document,t);if(solved.state=="error")throw std::runtime_error(solved.error);t=solved.sketch;
            std::vector<std::string> ps=refs;for(const auto& e:t.entities)if(e.id==refs.front()){ps=e.points;if(e.kind=="circle"||e.kind=="arc")values.push_back(e.radius);break;}
            std::vector<double> coordinates;for(const auto& id:ps){auto it=std::find_if(t.points.begin(),t.points.end(),[&](const auto& q){return q.id==id;});if(it==t.points.end())throw std::runtime_error("Punto inesistente: "+id);coordinates.push_back(it->x);coordinates.push_back(it->y);}coordinates.insert(coordinates.end(),values.begin(),values.end());values=coordinates;
        }auto id=constraint(t,kind,refs,values);return store(s,f,t,{{"constraint",id}});
    }));
    specs.push_back(spec("sketch.dimension.add","Quota schizzo","{sketch?: id, kind: distance|length|diameter|radius|angle, refs: [id,...], expression: string}",[](Session& s,const J& p){
        only_keys(p,{"sketch","kind","refs","expression"});auto& f=feature(s,p);auto t=doc::read_sketch(f.data);auto kind=string_arg(p,"kind");const std::vector<std::string> kinds={"distance","length","diameter","radius","angle"};
        if(std::find(kinds.begin(),kinds.end(),kind)==kinds.end())throw std::runtime_error("Tipo di quota sconosciuto");auto refs=p.at("refs").get<std::vector<std::string>>();auto id=dimension(t,kind,refs,string_arg(p,"expression"));return store(s,f,t,{{"dimension",id}});
    },true,"D"));
    specs.push_back(spec("sketch.dimension.set","Modifica quota","{sketch?: id, dimension: id, expression: string}",[](Session& s,const J& p){
        only_keys(p,{"sketch","dimension","expression"});auto& f=feature(s,p);auto t=doc::read_sketch(f.data);auto id=string_arg(p,"dimension");auto it=std::find_if(t.dimensions.begin(),t.dimensions.end(),[&](const auto& q){return q.id==id;});if(it==t.dimensions.end())throw std::runtime_error("Quota inesistente: "+id);it->expression=string_arg(p,"expression");return store(s,f,t);
    }));
    specs.push_back(spec("sketch.point.move","Sposta punto","{sketch?: id, point: id, x: mm, y: mm} (coalesce per trascinamento)",[](Session& s,const J& p){
        only_keys(p,{"sketch","point","x","y"});auto& f=feature(s,p);auto t=doc::read_sketch(f.data);auto id=string_arg(p,"point");if(!p.contains("x")||!p.contains("y"))throw std::runtime_error("Mancano le coordinate");
        if(std::none_of(t.points.begin(),t.points.end(),[&](const auto& q){return q.id==id;}))throw std::runtime_error("Punto inesistente: "+id);
        auto r=solver::drag(s.document,t,id,coordinate(p,"x"),coordinate(p,"y"));return store(s,f,r.sketch);
    }));
    return specs;
}
}
