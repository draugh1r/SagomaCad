#include "solver/sketch_solver.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace sagomacad;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
doc::Sketch rectangle(){
    doc::Sketch s;s.points={{"p0",0,0},{"p1",38,1},{"p2",39,19},{"p3",-1,20}};
    s.entities={{"a","line",{"p0","p1"}},{"b","line",{"p1","p2"}},{"c","line",{"p2","p3"}},{"d","line",{"p3","p0"}}};
    s.constraints={{"h0","horizontal",{"a"}},{"h1","horizontal",{"c"}},{"v0","vertical",{"b"}},{"v1","vertical",{"d"}},
        {"origin","fix",{"p0"},{},{0,0}}};
    s.dimensions={{"width","length",{"a"},"40 mm"},{"height","length",{"b"},"20 mm"}};return s;
}
int main(){try{
    doc::Document d;auto s=rectangle();auto r=solver::solve(d,s);
    check(r.state=="constrained"&&r.degrees_of_freedom==0&&r.profiles.size()==1,"rettangolo completamente vincolato e profilo chiuso");
    check(std::abs(r.sketch.points[2].x-40)<1e-5&&std::abs(r.sketch.points[2].y-20)<1e-5,"rettangolo 40x20");
    check(doc::sketch_json(doc::read_sketch(doc::sketch_json(s)))==doc::sketch_json(s),"roundtrip sketch");
    s=rectangle();s.constraints.pop_back();r=solver::solve(d,s);check(r.degrees_of_freedom==2,"traslazione libera DOF2");
    r=solver::drag(d,r.sketch,"p0",12,17);check(r.state=="free"&&r.degrees_of_freedom==2,"drag mantiene DOF");
    check(std::abs(r.sketch.points[2].x-52)<1e-4&&std::abs(r.sketch.points[2].y-37)<1e-4,"trascino un punto e il rettangolo segue");
    doc::Sketch rail;rail.points={{"origin",0,0},{"free",10,0}};rail.constraints={{"origin-fixed","fix",{"origin"},{},{0,0}}};rail.dimensions={{"on-circle","distance",{"origin","free"},"10 mm"}};
    r=solver::drag(d,rail,"free",10,10);
    check(r.state=="free"&&r.degrees_of_freedom==1&&r.residual<1e-5,"drag parzialmente vincolato");
    check(std::abs(r.sketch.points[1].x-r.sketch.points[1].y)<1e-4,"drag segue curva senza richiedere cursore esattamente sulla curva");
    s=rectangle();s.constraints.push_back({"h-extra","horizontal",{"a"}});r=solver::solve(d,s);
    check(r.state=="constrained"&&r.redundant==std::vector<std::string>{"h-extra"},"vincolo ridondante identificato");
    s.dimensions.push_back({"bad-width","length",{"a"},"60 mm"});r=solver::solve(d,s);
    check(r.state=="conflict"&&r.conflicting.size()==2&&r.error.find("bad-width")!=std::string::npos,"conflitto identificato");
    s={};s.points={{"o",0,0}};s.entities={{"circle","circle",{"o"},4}};s.constraints={{"fix","fix",{"o"},{},{0,0}}};
    s.dimensions={{"dia","diameter",{"circle"},"foro"}};d.parameters={{"foro","10 mm","mm"}};
    r=solver::solve(d,s);check(r.state=="constrained"&&std::abs(r.sketch.entities[0].radius-5)<1e-5,"cerchio parametrico");
    d.parameters[0].expression="18 mm";r=solver::solve(d,r.sketch);check(std::abs(r.sketch.entities[0].radius-9)<1e-5,"aggiornamento parametro");
    s.dimensions[0].expression="1 deg";check(solver::solve(d,s).state=="error","unità incompatibili");
    // Each equation is exercised independently, away from singular initial configurations.
    auto equation=[&](std::string kind,std::vector<std::string> refs,std::string expression="",std::vector<double> values={}){
        doc::Sketch t;t.points={{"p0",0,0},{"p1",12,2},{"p2",3,10},{"p3",14,13},{"mid",4,1},{"center",6,7}};
        t.entities={{"l0","line",{"p0","p1"}},{"l1","line",{"p2","p3"}},{"circle","circle",{"center"},3},{"circle2","circle",{"mid"},2}};
        t.constraints={{"tested",kind,refs,expression,values}};auto q=solver::solve(d,t);
        if(q.state=="error"||q.state=="conflict")throw std::runtime_error(kind+": "+q.error);
        check(q.residual<1e-5,"residuo equazione");
    };
    equation("coincident",{"p0","p2"});equation("horizontal",{"l0"});equation("vertical",{"l0"});
    equation("parallel",{"l0","l1"});equation("perpendicular",{"l0","l1"});equation("midpoint",{"mid","l0"});
    equation("equal",{"l0","l1"});equation("equal",{"circle","circle2"});equation("tangent",{"l0","circle"});equation("tangent",{"circle","circle2"});
    equation("distance",{"p0","p2"},"20 mm");equation("length",{"l0"},"21 mm");equation("radius",{"circle"},"7 mm");
    equation("angle",{"l0"},"30 deg");equation("angle",{"l0","l1"},"60 deg");equation("fix",{"l0"},"",{1,2,20,3});
    doc::Sketch arc;arc.points={{"o",0,0},{"start",8,0},{"end",0,12}};arc.entities={{"arc","arc",{"o","start","end"},10}};
    arc.dimensions={{"radius","radius",{"arc"},"10 mm"}};r=solver::solve(d,arc);check(r.state=="free"&&r.residual<1e-5,"arco equazioni interne");
    std::cout<<"Solver: equazioni, DOF, ridondanze, conflitti, parametri, drag, profili OK\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
