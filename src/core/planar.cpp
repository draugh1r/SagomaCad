#include "core/planar.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <stdexcept>

namespace sagomacad::core::planar {
namespace {
constexpr double tau=2*std::numbers::pi,eps=1e-8;
Point sub(Point a,Point b){return {a[0]-b[0],a[1]-b[1]};}
double cross(Point a,Point b){return a[0]*b[1]-a[1]*b[0];}
double dot(Point a,Point b){return a[0]*b[0]+a[1]*b[1];}
double norm(Point a){return std::hypot(a[0],a[1]);}
std::string pack(const std::string& text){return std::to_string(text.size())+":"+text;}
double angle(Point a){return std::atan2(a[1],a[0]);}
double normalize(double t){t=std::fmod(t,tau);return t<0?t+tau:t;}
double parameter(const Curve& c,Point p){
    if(c.line){auto d=sub(c.b,c.a);return dot(sub(p,c.a),d)/dot(d,d);}
    double t=normalize(angle(sub(p,c.center)));if(!c.closed&&t<c.begin-eps)t+=tau;return t;
}
bool accepts(const Curve& c,Point p){double t=parameter(c,p);return c.closed||(t>=c.begin-eps&&t<=c.end+eps);}
std::vector<Point> intersections(const Curve& a,const Curve& b){
    std::vector<Point> out;
    if(a.line&&b.line){auto u=sub(a.b,a.a),v=sub(b.b,b.a),d=sub(b.a,a.a);double den=cross(u,v);if(std::abs(den)>eps){double t=cross(d,v)/den;out.push_back(evaluate(a,t));}}
    else if(a.line||b.line){
        const auto& l=a.line?a:b;const auto& c=a.line?b:a;auto d=sub(l.b,l.a),o=sub(l.a,c.center);double A=dot(d,d),B=2*dot(o,d),C=dot(o,o)-c.radius*c.radius,disc=B*B-4*A*C;
        if(disc>eps*A){double root=std::sqrt(disc);out.push_back(evaluate(l,(-B-root)/(2*A)));out.push_back(evaluate(l,(-B+root)/(2*A)));}
    }else{
        auto delta=sub(b.center,a.center);double d=norm(delta);
        if(d>eps&&d<a.radius+b.radius-eps&&d>std::abs(a.radius-b.radius)+eps){
            double x=(a.radius*a.radius-b.radius*b.radius+d*d)/(2*d),h=std::sqrt(std::max(0.,a.radius*a.radius-x*x));
            Point base{a.center[0]+x*delta[0]/d,a.center[1]+x*delta[1]/d};
            out.push_back({base[0]-h*delta[1]/d,base[1]+h*delta[0]/d});out.push_back({base[0]+h*delta[1]/d,base[1]-h*delta[0]/d});
        }
    }
    // Keep branch indices before filtering arc/line domains: identity must not
    // switch when just one of the analytic roots crosses an endpoint.
    return out;
}
double integral(const Curve& c,double a,double b){
    if(c.line)return cross(evaluate(c,a),evaluate(c,b))/2;
    return (c.radius*c.center[0]*(std::sin(b)-std::sin(a))+c.radius*c.center[1]*(std::cos(a)-std::cos(b))+c.radius*c.radius*(b-a))/2;
}
}
Curve line(std::string id,Point a,Point b,std::string first,std::string last){Curve c;c.id=std::move(id);c.line=true;c.a=a;c.b=b;c.first=std::move(first);c.last=std::move(last);c.end=1;return c;}
Curve circle(std::string id,Point center,double radius){Curve c;c.id=std::move(id);c.closed=true;c.center=center;c.radius=radius;c.end=tau;return c;}
Curve arc(std::string id,Point center,double radius,Point a,Point b,std::string first,std::string last){auto c=circle(std::move(id),center,radius);c.closed=false;c.begin=normalize(angle(sub(a,center)));c.end=normalize(angle(sub(b,center)));if(c.end<=c.begin)c.end+=tau;c.first=std::move(first);c.last=std::move(last);return c;}
Point evaluate(const Curve& c,double t){return c.line?Point{c.a[0]+t*(c.b[0]-c.a[0]),c.a[1]+t*(c.b[1]-c.a[1])}:Point{c.center[0]+c.radius*std::cos(t),c.center[1]+c.radius*std::sin(t)};}
Arrangement arrange(std::vector<Curve> curves){
    if(curves.size()>2048)throw std::runtime_error("Too many curves for planar arrangement (2048)");
    Arrangement out;std::sort(curves.begin(),curves.end(),[](const auto& a,const auto& b){return a.id<b.id;});out.curves=std::move(curves);
    struct Node {Point p;std::string label;std::vector<std::size_t> outgoing;};std::vector<Node> nodes;
    auto node=[&](Point p,std::string label){
        for(std::size_t i=0;i<nodes.size();++i)if(norm(sub(nodes[i].p,p))<eps){if(label<nodes[i].label)nodes[i].label=label;return i;}
        if(nodes.size()>=8192)throw std::runtime_error("Too many planar intersections (8192)");nodes.push_back({p,std::move(label),{}});return nodes.size()-1;
    };
    std::vector<std::vector<std::pair<double,std::size_t>>> cuts(out.curves.size());
    for(std::size_t i=0;i<out.curves.size();++i){const auto& c=out.curves[i];
        if(c.id.empty()||c.id.size()>512||(!c.line&&(!std::isfinite(c.radius)||c.radius<=eps)))throw std::runtime_error("Invalid planar curve");
        for(const auto& p:{evaluate(c,c.begin),evaluate(c,c.end)})if(!std::isfinite(p[0])||!std::isfinite(p[1])||norm(p)>1e8)throw std::runtime_error("Invalid planar coordinate");
        if(c.line&&norm(sub(c.a,c.b))<eps)throw std::runtime_error("Degenerate planar line");
        if(!c.closed){cuts[i].push_back({c.begin,node(evaluate(c,c.begin),"p"+pack(c.first))});cuts[i].push_back({c.end,node(evaluate(c,c.end),"p"+pack(c.last))});}
    }
    for(std::size_t i=0;i<out.curves.size();++i)for(std::size_t j=i+1;j<out.curves.size();++j){
        const auto& a=out.curves[i];const auto& b=out.curves[j];const auto points=intersections(a,b);
        for(std::size_t branch=0;branch<points.size();++branch){auto p=points[branch];if(!accepts(a,p)||!accepts(b,p))continue;
            const auto n=node(p,"i"+pack(a.id)+pack(b.id)+std::to_string(branch));cuts[i].push_back({parameter(a,p),n});cuts[j].push_back({parameter(b,p),n});}
    }
    struct Half {std::size_t span,from,to;bool forward;double tangent;};std::vector<Half> half;
    for(std::size_t i=0;i<out.curves.size();++i){auto& values=cuts[i];const auto& c=out.curves[i];
        std::sort(values.begin(),values.end());values.erase(std::unique(values.begin(),values.end(),[](const auto& a,const auto& b){return a.second==b.second;}),values.end());
        if(c.closed&&values.empty())values.push_back({0,node(evaluate(c,0),"s"+pack(c.id))});
        if((c.closed&&values.size()>1)||(!c.closed&&values.size()>2))out.split.insert(c.id);
        const std::size_t count=c.closed?values.size():values.size()-1;
        for(std::size_t k=0;k<count;++k){auto [a,from]=values[k];auto [b,to]=values[(k+1)%values.size()];if(c.closed&&b<=a)b+=tau;if(b-a<eps)continue;
            if(out.spans.size()>=16384)throw std::runtime_error("Too many planar spans (16384)");
            const auto span=out.spans.size();out.spans.push_back({i,a,b,pack(c.id)+pack(nodes[from].label)+pack(nodes[to].label)});
            double forward=c.line?angle(sub(c.b,c.a)):a+std::numbers::pi/2,reverse=c.line?forward+std::numbers::pi:b-std::numbers::pi/2;
            nodes[from].outgoing.push_back(half.size());half.push_back({span,from,to,true,normalize(forward)});
            nodes[to].outgoing.push_back(half.size());half.push_back({span,to,from,false,normalize(reverse)});
        }
    }
    for(auto& n:nodes)std::sort(n.outgoing.begin(),n.outgoing.end(),[&](auto a,auto b){return half[a].tangent==half[b].tangent?a<b:half[a].tangent<half[b].tangent;});
    std::vector<bool> seen(half.size());
    for(std::size_t start=0;start<half.size();++start){if(seen[start])continue;std::vector<std::size_t> cycle;std::size_t cursor=start;
        while(!seen[cursor]){seen[cursor]=true;cycle.push_back(cursor);const auto& outgoing=nodes[half[cursor].to].outgoing;const auto reverse=std::find(outgoing.begin(),outgoing.end(),cursor^1);if(reverse==outgoing.end())throw std::runtime_error("Invalid planar adjacency");const auto at=std::size_t(reverse-outgoing.begin());cursor=outgoing[(at+outgoing.size()-1)%outgoing.size()];}
        if(cursor!=start)continue;
        // Dangling strokes walk out and back along the same span; remove those
        // bridges rather than adding a slit to the exact face wire.
        std::set<std::size_t> members(cycle.begin(),cycle.end());std::vector<DirectedSpan> face;double area=0;bool split=false;
        for(auto h:cycle){if(members.contains(h^1))continue;const auto& edge=half[h];const auto& span=out.spans[edge.span];const auto& c=out.curves[span.curve];area+=(edge.forward?1:-1)*integral(c,span.begin,span.end);face.push_back({edge.span,edge.forward});split=split||out.split.contains(c.id);}
        if(area>eps&&split&&!face.empty())out.faces.push_back(std::move(face));
    }
    return out;
}
std::vector<Point> sample(const Arrangement& a,const std::vector<DirectedSpan>& face){
    std::vector<Point> out;
    for(const auto& edge:face){const auto& span=a.spans.at(edge.span);const auto& c=a.curves.at(span.curve);int count=c.line?1:std::max(2,int(std::ceil((span.end-span.begin)/tau*256)));
        if(out.size()+count>524288)throw std::runtime_error("Planar polygon exceeds sample budget");
        for(int k=0;k<count;++k){double t=double(k)/count;if(!edge.forward)t=1-t;out.push_back(evaluate(c,span.begin+(span.end-span.begin)*t));}
    }return out;
}
std::string selector(const Span& span,bool forward){return std::string(forward?"@+":"@-")+span.token;}
}
