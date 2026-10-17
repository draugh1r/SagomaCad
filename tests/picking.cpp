#include "engine/picking.hpp"
#include "kernel/shape.hpp"
#include <iostream>
#include <set>
#include <stdexcept>
using namespace sagomacad::engine;
void check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
int main(){try{
    using namespace sagomacad::kernel;
    auto box=make_box(20,30,40);check(bool(box),"box");auto mesh=tessellate(*box.value,.1);check(bool(mesh),"mesh");
    std::set<int> edges,faces;for(const auto& t:mesh.value->triangles)faces.insert(t.face_id);
    for(const auto& e:mesh.value->edges){check(e.edge_id>0,"missing edge ID");edges.insert(e.edge_id);}
    check(edges.size()==12&&mesh.value->edges.size()==12&&faces.size()==6,"unique OCCT topology IDs");
    PickRef front{PickKind::Face,"box",1},back{PickKind::Face,"box",2},edge{PickKind::Edge,"box",7},sketch{PickKind::Sketch,"sketch",1};
    std::vector<PickTriangle> ts={{{PickPoint{0,0,8},PickPoint{30,0,8},PickPoint{0,30,8}},back},{{PickPoint{0,0,4},PickPoint{30,0,4},PickPoint{0,30,4}},front}};
    std::vector<PickSegment> ss={{{PickPoint{0,5,4},PickPoint{30,5,4}},edge},{{PickPoint{0,20,2},PickPoint{30,20,2}},sketch}};
    IdBuffer buffer;buffer.build(40,40,ts,ss);
    check(buffer.at(5,10)==front,"frontmost face");check(buffer.at(5,5)==edge,"coplanar edge priority and radius");
    check(buffer.at(5,20)==sketch,"sketch filter");check(!buffer.at(-1,4)&&!buffer.at(39,39),"background/bounds");
    auto all=buffer.all(5,5);check(all.size()==3,"select other includes occluded unique entities");check(all.back().ref==back,"depth ordering");
    buffer.build(40,40,ts,{{{PickPoint{0,5,9},PickPoint{30,5,9}},edge}});check(buffer.at(5,5)==front,"hidden edges occluded");
    buffer.build(40,40,ts,ss,{false,false,true,false});auto body=front;body.kind=PickKind::Body;body.topology_id=0;
    check(buffer.at(5,5)==body&&buffer.all(5,5).size()==1,"body ID grouping");
    buffer.build(40,40,ts,ss,{false,true,false,false});check(buffer.at(5,5)==edge&&!buffer.at(5,20),"edges only");
    std::vector<PickRef> selection;select_pick(selection,front,false);select_pick(selection,edge,true);check(selection.size()==2,"Ctrl/Shift add");
    select_pick(selection,front,true);check(selection.size()==1&&selection[0]==edge,"Ctrl/Shift toggle");select_pick(selection,{},false);check(selection.empty(),"clear selection");
    bool invalid=false;try{buffer.build(100000,100000,{},{});}catch(...){invalid=true;}check(invalid,"allocation bound");
    std::cout<<"Picking: 6 faces, 12 unique edges, ID/depth, occlusion, deep selection, filters and additive selection OK\n";
}catch(const std::exception& e){std::cerr<<e.what();return 1;}}
