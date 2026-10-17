#include "engine/tools/sketch_overlay.hpp"
#include <cmath>
#include <numbers>
#include <map>
#include <algorithm>
namespace sagomacad::engine {
SketchOverlay sketch_overlay(const solver::SketchResult& result){
    SketchOverlay out;std::map<std::string,SketchXY> points;std::map<std::string,const doc::SketchEntity*> entities;
    for(const auto& p:result.sketch.points)points[p.id]={p.x,p.y};
    for(const auto& e:result.sketch.entities){entities[e.id]=&e;SketchStroke stroke;stroke.id=e.id;stroke.construction=e.construction;stroke.reference=!e.reference_name.empty();stroke.fixed=true;
        for(const auto& p:e.points)if(!result.fixed_points.contains(p)||!result.fixed_points.at(p))stroke.fixed=false;
        if((e.kind=="circle"||e.kind=="arc")&&result.state!="constrained"){
            bool radius=false;for(const auto& d:result.sketch.dimensions)if(!d.refs.empty()&&d.refs[0]==e.id&&(d.kind=="radius"||d.kind=="diameter"))radius=true;
            for(const auto& c:result.sketch.constraints)if(c.kind=="fix"&&!c.refs.empty()&&c.refs[0]==e.id)radius=true;stroke.fixed=stroke.fixed&&radius;
        }
        if(e.kind=="circle"||e.kind=="arc"){
            auto c=points.at(e.points[0]);double start=0,end=2*std::numbers::pi;
            if(e.kind=="arc"){auto a=points.at(e.points[1]),b=points.at(e.points[2]);start=std::atan2(a.y-c.y,a.x-c.x);end=std::atan2(b.y-c.y,b.x-c.x);while(end<=start)end+=2*std::numbers::pi;}
            for(int i=0;i<=96;++i){double a=start+(end-start)*i/96;stroke.points.push_back({c.x+e.radius*std::cos(a),c.y+e.radius*std::sin(a)});}
        }else for(const auto& p:e.points)stroke.points.push_back(points.at(p));
        out.strokes.push_back(stroke);
    }
    for(const auto& profile:result.profiles){std::vector<SketchXY> vertices;
        for(const auto& id:profile){auto it=std::find_if(out.strokes.begin(),out.strokes.end(),[&](const auto& s){return s.id==id;});if(it==out.strokes.end())continue;
            auto pts=it->points;if(!vertices.empty()&&pts.size()>1){auto last=vertices.back();auto a=pts.front(),b=pts.back();if(std::hypot(last.x-b.x,last.y-b.y)<std::hypot(last.x-a.x,last.y-a.y))std::reverse(pts.begin(),pts.end());}
            vertices.insert(vertices.end(),pts.begin(),pts.end());
        }if(vertices.size()>=3)out.profiles.push_back(vertices);
    }
    for(const auto& d:result.sketch.dimensions){if(d.refs.empty())continue;SketchXY p;auto found=entities.find(d.refs[0]);
        if(found!=entities.end()){const auto& e=*found->second;p=points.at(e.points[0]);if(e.kind=="line"){auto b=points.at(e.points[1]);p={(p.x+b.x)/2,(p.y+b.y)/2};}else p.x+=e.radius;}
        else if(points.contains(d.refs[0])){p=points.at(d.refs[0]);if(d.refs.size()==2&&points.contains(d.refs[1])){auto q=points.at(d.refs[1]);p={(p.x+q.x)/2,(p.y+q.y)/2};}}
        bool vertical=false;if(found!=entities.end()&&found->second->kind=="line"){auto a=points.at(found->second->points[0]),b=points.at(found->second->points[1]);vertical=std::abs(b.y-a.y)>std::abs(b.x-a.x);}
        out.dimensions.push_back({d.id,d.expression,d.kind,p,vertical});
    }
    for(const auto& c:result.sketch.constraints){if(c.refs.empty())continue;SketchXY p;auto found=entities.find(c.refs[0]);if(found!=entities.end()){const auto& e=*found->second;p=points.at(e.points[0]);if(e.points.size()>1){auto b=points.at(e.points[1]);p={(p.x+b.x)/2,(p.y+b.y)/2};}}else if(points.contains(c.refs[0]))p=points.at(c.refs[0]);
        const auto text=c.kind=="horizontal"?"-":c.kind=="vertical"?"|":c.kind=="fix"?"+":c.kind=="coincident"?"o":c.kind=="equal"?"=":c.kind=="parallel"?"//":c.kind=="perpendicular"?"L":c.kind=="tangent"?"T":c.kind=="midpoint"?"M":"+";
        out.constraints.push_back({c.id,text,c.kind,p});}
    return out;
}
}
