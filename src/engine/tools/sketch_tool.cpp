#include "engine/tools/sketch_tool.hpp"
#include <cmath>
#include <numbers>
#include <sstream>
#include "doc/expression.hpp"
namespace sagomacad::engine {
namespace {std::string numeric(double v){std::ostringstream s;s.precision(8);s<<v;return s.str();}}
void SketchTool::begin(std::string tool){kind=std::move(tool);started=false;fields={};locked={false,false};active_field=0;arc_stage=0;arc_radius=0;error.clear();}
void SketchTool::move(double u,double v){cursor_x=u;cursor_y=v;}
void SketchTool::click(double u,double v){move(u,v);if(!started){started=true;x=u;y=v;fields={};locked={false,false};active_field=0;}}
void SketchTool::text(std::string_view value){if(started){if(!locked[active_field])fields[active_field].clear();fields[active_field].append(value);locked[active_field]=true;}}
void SketchTool::tab(){if(started)active_field=(active_field+1)%field_count();}
void SketchTool::escape(){if(started){started=false;fields={};locked={false,false};arc_stage=0;}else begin("select");error.clear();}
std::string SketchTool::hint()const{if(kind=="select")return "Seleziona una geometria o trascina un punto";if(kind=="dimension")return "Seleziona una geometria per quotarla";if(!started)return kind=="circle"||kind=="arc"?"Posiziona il centro":"Posiziona il primo punto";return kind=="arc"&&arc_stage==0?"Posiziona il primo punto dell'arco":"Clic per confermare · Digita una quota per bloccarla · Tab cambia campo";}
std::array<std::string,2> SketchTool::live_values()const{
    auto display=[](double value){std::ostringstream out;out<<std::fixed;out.precision(3);out<<value;auto text=out.str();while(text.back()=='0')text.pop_back();if(text.back()=='.')text.pop_back();return text=="-0"?std::string("0"):text;};
    double dx=cursor_x-x,dy=cursor_y-y,r=std::hypot(dx,dy),angle=std::atan2(dy,dx)*180/std::numbers::pi;
    if(kind=="rect")return {display(std::abs(dx)),display(std::abs(dy))};
    if(kind=="circle")return {display(2*r),{}};
    if(kind=="arc")return {display(arc_stage?arc_radius:r),display(angle)};
    return {display(r),display(angle)};
}
std::vector<SketchMeasure> SketchTool::measurements(const solver::SketchResult& solved)const{
    std::vector<SketchMeasure> out;if(!started)return out;
    if(solved.sketch.entities.empty()){
        SketchMeasure m;m.a={x,y};m.b={cursor_x,cursor_y};m.position={(x+cursor_x)/2,(y+cursor_y)/2};m.outward={cursor_y-y,x-cursor_x};m.label="Lunghezza";m.unit="mm";
        if(kind=="rect"){m.b.y=y;m.position.y=y;m.outward={0,-1};m.label="Larghezza";}
        else if(kind=="circle"||kind=="arc")m.label=kind=="circle"?"Diametro":"Raggio";
        out.push_back(m);
        if(field_count()>1){m.field=1;m.label=kind=="rect"?"Altezza":"Angolo";m.unit=kind=="rect"?"mm":"deg";m.outward={1,0};m.position={cursor_x,(y+cursor_y)/2};out.push_back(m);}
        return out;
    }
    auto point=[&](const std::string& id){for(const auto& p:solved.sketch.points)if(p.id==id)return SketchXY{p.x,p.y};return SketchXY{};};
    auto add=[&](int field,std::string label,std::string unit,SketchXY a,SketchXY b,std::vector<std::string> refs){
        SketchMeasure m;m.field=field;m.label=label;m.unit=unit;m.a=a;m.b=b;m.position={(a.x+b.x)/2,(a.y+b.y)/2};m.outward={b.y-a.y,a.x-b.x};m.refs=std::move(refs);out.push_back(m);
    };
    const auto& entities=solved.sketch.entities;const auto& e=entities.back();
    if(kind=="rect"&&entities.size()>=4){
        const auto& w=entities[entities.size()-4];const auto& h=entities.back();
        add(0,"Larghezza","mm",point(w.points[0]),point(w.points[1]),{w.id});
        add(1,"Altezza","mm",point(h.points[0]),point(h.points[1]),{h.id});
    }else if(kind=="circle"||(kind=="arc"&&arc_stage>0)){
        const auto c=point(e.points[0]);const double angle=kind=="circle"?std::atan2(cursor_y-y,cursor_x-x):arc_start*std::numbers::pi/180;
        const SketchXY end{c.x+e.radius*std::cos(angle),c.y+e.radius*std::sin(angle)};
        add(0,kind=="circle"?"Diametro":"Raggio","mm",kind=="circle"?SketchXY{2*c.x-end.x,2*c.y-end.y}:c,end,{e.id});
        if(kind=="circle")out.back().position=end;
        if(kind=="arc"){
            auto b=point(e.points.back());add(1,"Angolo","deg",c,b,{e.id});
            out.back().position={c.x+(b.x-c.x)*.7,c.y+(b.y-c.y)*.7};out.back().outward={b.x-c.x,b.y-c.y};
        }
    }else{
        auto a=point(e.points[0]),b=point(e.points[1]);add(0,kind=="arc"?"Raggio":"Lunghezza","mm",a,b,{e.id});
        if(kind!="arc"){add(1,"Angolo","deg",a,b,{e.id});out.back().position={a.x+(b.x-a.x)*.25,a.y+(b.y-a.y)*.25};out.back().outward={a.x-b.x,a.y-b.y};}
    }
    return out;
}
nlohmann::json SketchTool::parameters()const{
    auto field=[&](int i,double fallback){return fields[i].empty()?numeric(fallback):fields[i];};
    const double dx=cursor_x-x,dy=cursor_y-y;const bool anchor=std::hypot(x,y)<1e-7;
    if(kind=="rect")return {{"x",x},{"y",y},{"width",field(0,std::max(std::abs(dx),.01))},{"height",field(1,std::max(std::abs(dy),.01))},{"anchor",anchor},{"flip_x",dx<0},{"flip_y",dy<0},{"driving_width",locked[0]},{"driving_height",locked[1]}};
    if(kind=="circle")return {{"x",x},{"y",y},{"diameter",field(0,std::max(2*std::hypot(dx,dy),.01))},{"anchor",anchor},{"driving_diameter",locked[0]}};
    if(kind=="arc")return {{"x",x},{"y",y},{"radius",field(0,std::max(arc_stage?arc_radius:std::hypot(dx,dy),.01))},{"start_angle",numeric(arc_start)},{"end_angle",field(1,std::atan2(dy,dx)*180/std::numbers::pi)},{"anchor",anchor},{"driving_radius",locked[0]},{"driving_start_angle",false},{"driving_end_angle",locked[1]}};
    nlohmann::json p={{"x1",x},{"y1",y},{"x2",cursor_x},{"y2",cursor_y},{"anchor",anchor}};
    if(!fields[0].empty())p["length"]=fields[0];if(!fields[1].empty())p["angle"]=fields[1];return p;
}
nlohmann::json SketchTool::preview_command()const{
    if(kind=="arc"&&arc_stage==0){nlohmann::json p={{"x1",x},{"y1",y},{"x2",cursor_x},{"y2",cursor_y}};if(!fields[0].empty())p["length"]=fields[0];return {{"command","sketch.line"},{"params",p}};}
    return {{"command","sketch."+kind},{"params",parameters()}};
}
CommandResult SketchTool::confirm(Engine& engine){
    if(!started||kind=="select"||kind=="dimension")return CommandResult::failure("Nessuna geometria da confermare");
    if(kind=="arc"&&arc_stage==0){
        double radius=std::hypot(cursor_x-x,cursor_y-y);
        if(!fields[0].empty()){doc::Quantity q;if(!doc::evaluate_expression(engine.session().document,fields[0],q,error)||!std::isfinite(q.value)||q.value<=0||(q.dimension!=doc::Dimension::Length&&q.dimension!=doc::Dimension::Scalar)){if(error.empty())error="Raggio non valido";return CommandResult::failure(error);}radius=q.value;}
        arc_stage=1;arc_start=std::atan2(cursor_y-y,cursor_x-x)*180/std::numbers::pi;arc_radius=radius;return {true,{{"preview",true}},{}};
    }
    auto result=engine.execute("sketch."+kind,parameters());
    error=result.ok?result.value.value("solver",nlohmann::json::object()).value("error",std::string{}):result.error;
    if(result.ok){started=false;fields={};locked={false,false};arc_stage=0;}return result;
}
}
