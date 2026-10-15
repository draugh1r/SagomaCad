#include "engine/tools/sketch_tool.hpp"
#include <cmath>
#include <numbers>
#include <sstream>
namespace sagomacad::engine {
namespace {std::string numeric(double v){std::ostringstream s;s.precision(8);s<<v;return s.str();}}
void SketchTool::begin(std::string tool){kind=std::move(tool);started=false;fields={};locked={false,false};active_field=0;arc_stage=0;error.clear();}
void SketchTool::move(double u,double v){cursor_x=u;cursor_y=v;}
void SketchTool::click(double u,double v){move(u,v);if(!started){started=true;x=u;y=v;fields={};locked={false,false};active_field=0;}}
void SketchTool::text(std::string_view value){if(started){if(!locked[active_field])fields[active_field].clear();fields[active_field].append(value);locked[active_field]=true;}}
void SketchTool::tab(){if(started)active_field=(active_field+1)%(kind=="circle"?1:2);}
void SketchTool::escape(){if(started){started=false;fields={};locked={false,false};arc_stage=0;}else begin("select");error.clear();}
std::string SketchTool::hint()const{if(kind=="select")return "Seleziona una geometria o trascina un punto";if(kind=="dimension")return "Seleziona una geometria per quotarla";if(!started)return kind=="circle"||kind=="arc"?"Posiziona il centro":"Posiziona il primo punto";return kind=="arc"&&arc_stage==0?"Posiziona il primo punto dell'arco":"Digita la misura · Tab cambia campo · Invio conferma";}
nlohmann::json SketchTool::parameters()const{
    auto field=[&](int i,double fallback){return fields[i].empty()?numeric(fallback):fields[i];};
    const double dx=cursor_x-x,dy=cursor_y-y;const bool anchor=std::hypot(x,y)<1e-7;
    if(kind=="rect")return {{"x",x},{"y",y},{"width",field(0,std::max(std::abs(dx),.01))},{"height",field(1,std::max(std::abs(dy),.01))},{"anchor",anchor},{"flip_x",dx<0},{"flip_y",dy<0}};
    if(kind=="circle")return {{"x",x},{"y",y},{"diameter",field(0,std::max(2*std::hypot(dx,dy),.01))},{"anchor",anchor}};
    if(kind=="arc")return {{"x",x},{"y",y},{"radius",field(0,std::max(std::hypot(dx,dy),.01))},{"start_angle",numeric(arc_start)},{"end_angle",field(1,std::atan2(dy,dx)*180/std::numbers::pi)},{"anchor",anchor}};
    nlohmann::json p={{"x1",x},{"y1",y},{"x2",cursor_x},{"y2",cursor_y},{"anchor",anchor}};
    if(!fields[0].empty())p["length"]=fields[0];if(!fields[1].empty())p["angle"]=fields[1];return p;
}
CommandResult SketchTool::confirm(Engine& engine){
    if(!started||kind=="select"||kind=="dimension")return CommandResult::failure("Nessuna geometria da confermare");
    if(kind=="arc"&&arc_stage==0){arc_stage=1;arc_start=std::atan2(cursor_y-y,cursor_x-x)*180/std::numbers::pi;if(fields[0].empty()){fields[0]=numeric(std::hypot(cursor_x-x,cursor_y-y));locked[0]=true;}return {true,{{"preview",true}},{}};}
    auto result=engine.execute("sketch."+kind,parameters());
    error=result.ok?result.value.value("solver",nlohmann::json::object()).value("error",std::string{}):result.error;
    if(result.ok){started=false;fields={};locked={false,false};arc_stage=0;}return result;
}
}
