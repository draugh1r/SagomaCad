#include "engine/picking.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace sagomacad::engine {
namespace {
bool enabled(PickKind k,PickFilter f){return k==PickKind::Face?f.faces:k==PickKind::Edge?f.edges:k==PickKind::Body?f.bodies:f.sketches;}
bool finite(PickPoint p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.depth);}
std::optional<double> triangle_depth(const PickTriangle& t,double x,double y){
    auto [a,b,c]=t.points;double det=(b.y-c.y)*(a.x-c.x)+(c.x-b.x)*(a.y-c.y);
    if(std::abs(det)<1e-12)return {};
    double u=((b.y-c.y)*(x-c.x)+(c.x-b.x)*(y-c.y))/det,v=((c.y-a.y)*(x-c.x)+(a.x-c.x)*(y-c.y))/det,w=1-u-v;
    if(u< -1e-9||v< -1e-9||w< -1e-9)return {};return u*a.depth+v*b.depth+w*c.depth;
}
std::optional<double> segment_depth(const PickSegment& s,double x,double y,double r){
    auto [a,b]=s.points;double dx=b.x-a.x,dy=b.y-a.y,len=dx*dx+dy*dy;
    double u=len<1e-12?0:std::clamp(((x-a.x)*dx+(y-a.y)*dy)/len,0.,1.);
    if(std::hypot(x-a.x-u*dx,y-a.y-u*dy)>r)return {};return a.depth+u*(b.depth-a.depth);
}
}
void IdBuffer::build(int width,int height,std::vector<PickTriangle> ts,std::vector<PickSegment> ss,PickFilter filter,double radius){
    if(width<=0||height<=0||width>8192||height>8192||std::uint64_t(width)*height>16777216||!std::isfinite(radius)||radius<0||radius>32||ts.size()>2000000||ss.size()>2000000)
        throw std::runtime_error("Dimensioni del buffer di selezione non valide");
    for(const auto& t:ts)for(auto p:t.points)if(!finite(p))throw std::runtime_error("Triangolo di selezione non finito");
    for(const auto& s:ss)for(auto p:s.points)if(!finite(p))throw std::runtime_error("Segmento di selezione non finito");
    width_=width;height_=height;filter_=filter;radius_=radius;triangles_=std::move(ts);segments_=std::move(ss);
    refs_={{}};ids_.assign(std::size_t(width)*height,0);depths_.assign(ids_.size(),std::numeric_limits<double>::infinity());
    auto raster=[&](const auto& primitive,double margin,auto depth){
        PickRef ref=primitive.ref;
        if(ref.kind==PickKind::Face&&!filter.faces&&filter.bodies){ref.kind=PickKind::Body;ref.topology_id=0;}
        if(!enabled(ref.kind,filter))return;
        refs_.push_back(ref);auto id=std::uint32_t(refs_.size()-1);
        double xmin=width,ymin=height,xmax=0,ymax=0;
        for(auto p:primitive.points){xmin=std::min(xmin,p.x-margin);ymin=std::min(ymin,p.y-margin);xmax=std::max(xmax,p.x+margin);ymax=std::max(ymax,p.y+margin);}
        if(xmax<0||ymax<0||xmin>=width||ymin>=height)return;
        int left=int(std::max(0.,xmin)),top=int(std::max(0.,ymin)),right=int(std::min(double(width-1),xmax)),bottom=int(std::min(double(height-1),ymax));
        for(int y=top;y<=bottom;++y)for(int x=left;x<=right;++x)if(auto z=depth(primitive,x+.5,y+.5)){
            auto i=std::size_t(y)*width+x;double epsilon=1e-6*std::max(1.,std::abs(*z));
            if(*z<=depths_[i]+epsilon){depths_[i]=*z;ids_[i]=id;}
        }
    };
    for(const auto& t:triangles_)raster(t,0.,triangle_depth);
    for(const auto& s:segments_)raster(s,radius,[&](const auto& p,double x,double y){return segment_depth(p,x,y,radius);});
}
std::optional<PickRef> IdBuffer::at(int x,int y)const{
    if(x<0||y<0||x>=width_||y>=height_)return {};auto id=ids_[std::size_t(y)*width_+x];return id?std::optional(refs_[id]):std::nullopt;
}
std::vector<PickHit> IdBuffer::all(int x,int y)const{
    std::vector<PickHit> hits;if(x<0||y<0||x>=width_||y>=height_)return hits;
    auto add=[&](PickRef ref,std::optional<double> z){
        if(ref.kind==PickKind::Face&&!filter_.faces&&filter_.bodies){ref.kind=PickKind::Body;ref.topology_id=0;}
        if(!z||!enabled(ref.kind,filter_))return;
        auto it=std::find_if(hits.begin(),hits.end(),[&](const auto& h){return h.ref==ref;});
        if(it==hits.end())hits.push_back({ref,*z});else it->depth=std::min(it->depth,*z);
    };
    for(const auto& t:triangles_)add(t.ref,triangle_depth(t,x+.5,y+.5));
    for(const auto& s:segments_)add(s.ref,segment_depth(s,x+.5,y+.5,radius_));
    std::stable_sort(hits.begin(),hits.end(),[](const auto& a,const auto& b){return a.depth<b.depth;});return hits;
}
void select_pick(std::vector<PickRef>& selection,const std::optional<PickRef>& hit,bool additive){
    if(!additive)selection.clear();if(!hit)return;auto it=std::find(selection.begin(),selection.end(),*hit);
    if(it==selection.end())selection.push_back(*hit);else if(additive)selection.erase(it);
}
}
