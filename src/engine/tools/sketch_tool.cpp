#include "engine/tools/sketch_tool.hpp"
#include <cmath>
#include <numbers>
#include <sstream>
namespace sagomacad::engine {
namespace {std::string numeric(double v){std::ostringstream s;s.precision(8);s<<v;return s.str();}}
void SketchTool::begin(std::string tool){kind=std::move(tool);started=false;fields={};locked={false,false};active_field=0;arc_stage=0;arc_radius=0;error.clear();}
void SketchTool::move(double u,double v){cursor_x=u;cursor_y=v;}
void SketchTool::click(double u,double v){move(u,v);if(!started){started=true;x=u;y=v;fields={};locked={false,false};active_field=0;}}
void SketchTool::text(std::string_view value){if(started){if(!locked[active_field])fields[active_field].clear();fields[active_field].append(value);locked[active_field]=true;}}
void SketchTool::tab(){if(started)active_field=(active_field+1)%(kind=="circle"?1:2);}
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
    if(kind=="arc"&&arc_stage==0)return {{"command","sketch.line"},{"params",{{"x1",x},{"y1",y},{"x2",cursor_x},{"y2",cursor_y}}}};
    return {{"command","sketch."+kind},{"params",parameters()}};
}
CommandResult SketchTool::confirm(Engine& engine){
    if(!started||kind=="select"||kind=="dimension")return CommandResult::failure("Nessuna geometria da confermare");
    if(kind=="arc"&&arc_stage==0){arc_stage=1;arc_start=std::atan2(cursor_y-y,cursor_x-x)*180/std::numbers::pi;arc_radius=std::hypot(cursor_x-x,cursor_y-y);return {true,{{"preview",true}},{}};}
    auto result=engine.execute("sketch."+kind,parameters());
    error=result.ok?result.value.value("solver",nlohmann::json::object()).value("error",std::string{}):result.error;
    if(result.ok){started=false;fields={};locked={false,false};arc_stage=0;}return result;
}
}
