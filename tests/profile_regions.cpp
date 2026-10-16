#include "solver/sketch_solver.hpp"
#include "solver/profile_regions.hpp"
#include <stdexcept>
#include <iostream>
using namespace sagomacad;
void check(bool b,const char* s){if(!b)throw std::runtime_error(s);}
int main(){try{
    doc::Sketch s;s.points={{"a",-30,-20},{"b",30,-20},{"c",30,20},{"d",-30,20},{"o",0,0}};
    s.entities={{"ab","line",{"a","b"}},{"bc","line",{"b","c"}},{"cd","line",{"c","d"}},{"da","line",{"d","a"}},{"hole","circle",{"o"},5},{"inner","circle",{"o"},2}};
    auto solved=solver::solve({},s);auto regions=solver::profile_regions(solved.sketch,solved.profiles);
    check(regions.size()==3,"tre regioni annidate");
    for(const auto& r:regions){if(r.boundary.size()==4){check(r.holes.size()==1&&r.holes[0][0]=="hole","solo il figlio diretto è foro");check(solver::contains(r,20,0)&&!solver::contains(r,0,0),"picking corona");}else if(r.boundary[0]=="hole")check(r.holes.size()==1,"corona interna");else check(r.holes.empty()&&solver::contains(r,0,0),"disco interno");}
    s.points[1].x=40;s.points[2].x=40;s.entities[4].radius=8;
    auto next=solver::profile_regions(s,solver::solve({},s).profiles);
    check(next.size()==regions.size(),"regioni dopo modifica");for(size_t i=0;i<next.size();++i)check(next[i].id==regions[i].id,"ID indipendente dalle quote");
    s.entities.back().construction=true;check(solver::solve({},s).profiles.size()==2,"costruzione esclusa");
    s.entities.erase(s.entities.begin());check(solver::solve({},s).profiles.size()==1,"contorno aperto escluso");
    check(solver::regions_json(regions)[0].contains("boundary"),"ispezione serializzabile");
    std::cout<<"Regioni annidate, picking, riferimenti stabili OK\n";
}catch(const std::exception& e){std::cerr<<e.what();return 1;}}
