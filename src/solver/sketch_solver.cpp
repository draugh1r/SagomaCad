#include "solver/sketch_solver.hpp"
#include "doc/expression.hpp"
#include <Eigen/Dense>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <set>
#include <stdexcept>

namespace sagomacad::solver {
namespace {
using V=Eigen::VectorXd;using M=Eigen::MatrixXd;using P=Eigen::Vector2d;
struct System {
    const doc::Document& document;doc::Sketch sketch;
    std::map<std::string,int> points,radii;
    std::map<std::string,const doc::SketchEntity*> entities;
    std::vector<std::pair<std::string,int>> blocks;
    V initial;
    System(const doc::Document& d,const doc::Sketch& s):document(d),sketch(doc::read_sketch(doc::sketch_json(s))){
        int n=0;for(const auto& p:sketch.points){points[p.id]=n;n+=2;}
        for(const auto& e:sketch.entities){entities[e.id]=&e;if(e.kind=="circle"||e.kind=="arc")radii[e.id]=n++;}
        initial=V::Zero(n);for(const auto& p:sketch.points){int i=points.at(p.id);initial[i]=p.x;initial[i+1]=p.y;}
        for(const auto& e:sketch.entities)if(radii.contains(e.id))initial[radii.at(e.id)]=e.radius;
    }
    P point(const V& x,const std::string& id)const {auto r=id;if(!points.contains(r)){const auto& e=*entities.at(r);r=e.points.front();}int i=points.at(r);return {x[i],x[i+1]};}
    const doc::SketchEntity& entity(const std::string& id,const std::string& kind="")const {
        const auto& e=*entities.at(id);if(!kind.empty()&&e.kind!=kind)throw std::runtime_error("Tipo di entità incompatibile: "+id);return e;
    }
    P direction(const V& x,const std::string& id)const{const auto& e=entity(id,"line");return point(x,e.points[1])-point(x,e.points[0]);}
    double radius(const V& x,const std::string& id)const{return x[radii.at(id)];}
    double value(const doc::SketchConstraint& c)const {
        doc::Quantity q;std::string error;
        if(!doc::evaluate_expression(document,c.expression,q,error))throw std::runtime_error(c.id+": "+error);
        const bool angle=c.kind=="angle";
        if(q.dimension!=(angle?doc::Dimension::Angle:doc::Dimension::Length)&&q.dimension!=doc::Dimension::Scalar)
            throw std::runtime_error(c.id+": unità incompatibili");
        if(!std::isfinite(q.value)||std::abs(q.value)>1e7)throw std::runtime_error(c.id+": quota fuori intervallo");
        return angle&&q.dimension==doc::Dimension::Scalar?q.value*std::numbers::pi/180:q.value;
    }
    V residual(const V& x){
        std::vector<double> r;blocks.clear();
        auto push=[&](P p){r.push_back(p.x());r.push_back(p.y());};
        for(const auto& e:sketch.entities)if(e.kind=="arc"){
            const auto c=point(x,e.points[0]);r.push_back((point(x,e.points[1])-c).norm()-radius(x,e.id));
            r.push_back((point(x,e.points[2])-c).norm()-radius(x,e.id));blocks.push_back({"internal:"+e.id,static_cast<int>(r.size())});
        }
        auto evaluate=[&](const doc::SketchConstraint& c){
            auto ref=[&](size_t i)->const std::string&{if(i>=c.refs.size())throw std::runtime_error(c.id+": riferimenti insufficienti");return c.refs[i];};
            auto p=[&](size_t i){return point(x,ref(i));};auto dir=[&](size_t i){return direction(x,ref(i));};
            auto len=[&](size_t i){return dir(i).norm();};
            const auto& k=c.kind;
            if(k=="coincident")push(p(0)-p(1));
            else if(k=="horizontal")r.push_back(dir(0).y());
            else if(k=="vertical")r.push_back(dir(0).x());
            else if(k=="parallel"||k=="perpendicular"){
                auto a=dir(0),b=dir(1);const double scale=std::max({a.norm(),b.norm(),1.0});
                r.push_back((k=="parallel"?a.x()*b.y()-a.y()*b.x():a.dot(b))/scale);
            }else if(k=="midpoint"){
                const auto& e=entity(ref(1),"line");push(p(0)-(point(x,e.points[0])+point(x,e.points[1]))/2);
            }else if(k=="centered")push((p(0)+p(1))/2);
            else if(k=="fix"){
                const auto& id=ref(0);std::vector<double> actual;
                if(points.contains(id)){auto a=p(0);actual={a.x(),a.y()};}
                else{const auto& e=entity(id);for(const auto& pid:e.points){auto a=point(x,pid);actual.push_back(a.x());actual.push_back(a.y());}if(radii.contains(id))actual.push_back(radius(x,id));}
                if(actual.size()!=c.values.size())throw std::runtime_error(c.id+": coordinate del vincolo fisso mancanti");
                for(size_t i=0;i<actual.size();++i)r.push_back(actual[i]-c.values[i]);
            }else if(k=="equal"){
                const auto& a=entity(ref(0));const auto& b=entity(ref(1));
                if(a.kind=="line"&&b.kind=="line")r.push_back(len(0)-len(1));
                else r.push_back(radius(x,ref(0))-radius(x,ref(1)));
            }else if(k=="tangent"){
                const auto& a=entity(ref(0));const auto& b=entity(ref(1));
                if(a.kind=="line"||b.kind=="line"){
                    auto li=a.kind=="line"?0:1,ci=1-li;auto d=dir(li);const auto& line=entity(ref(li));
                    auto delta=p(ci)-point(x,line.points[0]);
                    r.push_back(std::abs(d.x()*delta.y()-d.y()*delta.x())/std::max(d.norm(),1e-10)-radius(x,ref(ci)));
                }else{double target=radius(x,ref(0))+radius(x,ref(1));if(!c.values.empty()&&c.values[0]<0)target=std::abs(radius(x,ref(0))-radius(x,ref(1)));r.push_back((p(0)-p(1)).norm()-target);}
            }else if(k=="distance")r.push_back((p(0)-p(1)).norm()-value(c));
            else if(k=="length")r.push_back(len(0)-value(c));
            else if(k=="radius"||k=="diameter"){double v=value(c);if(v<=0)throw std::runtime_error(c.id+": la quota deve essere positiva");r.push_back(radius(x,ref(0))*(k=="diameter"?2:1)-v);}
            else if(k=="angle"){
                double angle=0;
                if(c.refs.size()==2&&points.contains(ref(0))&&points.contains(ref(1))){auto a=p(1)-p(0);angle=std::atan2(a.y(),a.x());}
                else {auto a=dir(0);angle=std::atan2(a.y(),a.x());if(c.refs.size()>1){auto b=dir(1);angle=std::atan2(a.x()*b.y()-a.y()*b.x(),a.dot(b));}}
                r.push_back(std::remainder(angle-value(c),2*std::numbers::pi));
            }else throw std::runtime_error(c.id+": tipo di vincolo sconosciuto: "+k);
            blocks.push_back({c.id,static_cast<int>(r.size())});
        };
        for(const auto& c:sketch.constraints)evaluate(c);for(const auto& c:sketch.dimensions)evaluate(c);
        V out(r.size());for(size_t i=0;i<r.size();++i)out[i]=r[i];if(!out.allFinite())throw std::runtime_error("sketch.numeric: equazioni non finite");return out;
    }
    M jacobian(const V& x){V r=residual(x);M j(r.size(),x.size());for(int i=0;i<x.size();++i){V a=x,b=x;double h=1e-5*std::max(1.0,std::abs(x[i]));a[i]+=h;b[i]-=h;j.col(i)=(residual(a)-residual(b))/(2*h);}residual(x);return j;}
};
int rank(const M& j){if(!j.rows()||!j.cols())return 0;Eigen::JacobiSVD<M> svd(j);svd.setThreshold(1e-7);return static_cast<int>(svd.rank());}
void profiles(SketchResult& result){
    const auto& s=result.sketch;std::map<std::string,P> pts;for(const auto& p:s.points)pts[p.id]={p.x,p.y};
    std::map<std::string,std::string> canonical;for(const auto& p:s.points){canonical[p.id]=p.id;for(const auto& q:s.points)if(q.id<p.id&&(pts[p.id]-pts[q.id]).norm()<1e-5){canonical[p.id]=canonical[q.id];break;}}
    struct Edge{std::string id,a,b;};std::vector<Edge> edges;
    for(const auto& e:s.entities)if(!e.construction){if(e.kind=="circle")result.profiles.push_back({e.id});else if(e.kind=="line")edges.push_back({e.id,canonical[e.points[0]],canonical[e.points[1]]});else if(e.kind=="arc")edges.push_back({e.id,canonical[e.points[1]],canonical[e.points[2]]});}
    std::set<size_t> visited;
    for(size_t start=0;start<edges.size();++start){if(visited.contains(start))continue;std::vector<size_t> chain{start};std::string cursor=edges[start].b;bool closed=false;
        for(size_t step=0;step<edges.size();++step){if(cursor==edges[start].a){closed=true;break;}size_t next=edges.size();for(size_t i=0;i<edges.size();++i)if(std::find(chain.begin(),chain.end(),i)==chain.end()&&(edges[i].a==cursor||edges[i].b==cursor)){next=i;break;}if(next==edges.size())break;chain.push_back(next);cursor=edges[next].a==cursor?edges[next].b:edges[next].a;}
        if(closed){std::vector<std::string> ids;for(auto i:chain){visited.insert(i);ids.push_back(edges[i].id);}result.profiles.push_back(ids);}
    }
}
}
SketchResult solve(const doc::Document& document,const doc::Sketch& sketch){
    SketchResult out;out.sketch=sketch;
    try{
        System system(document,sketch);V x=system.initial,r=system.residual(x);double lambda=1e-3;
        for(int iteration=0;iteration<120&&r.squaredNorm()>1e-16&&x.size();++iteration){M j=system.jacobian(x);M a=j.transpose()*j;a.diagonal().array()+=lambda;V delta=a.ldlt().solve(-j.transpose()*r);if(!delta.allFinite())break;
            V candidate=x+delta;if(candidate.cwiseAbs().maxCoeff()>1e7){lambda*=10;continue;}V next=system.residual(candidate);
            if(next.squaredNorm()<r.squaredNorm()){x=candidate;r=next;lambda=std::max(lambda*.3,1e-12);if(delta.norm()<1e-10)break;}else{lambda*=10;if(lambda>1e15)break;}}
        r=system.residual(x);M j=system.jacobian(x);const auto blocks=system.blocks;const int total_rank=rank(j);
        out.residual=r.norm();out.degrees_of_freedom=static_cast<int>(x.size())-total_rank;
        out.state=r.norm()>1e-5?"conflict":out.degrees_of_freedom?"free":"constrained";
        int previous=0,previous_rank=0;
        for(const auto& [id,end]:blocks){const int current=rank(j.topRows(end));if(id.rfind("internal:",0)!=0){
            if((r.segment(previous,end-previous).array().abs()>1e-5).any())out.conflicting.push_back(id);
            else if(current-previous_rank<end-previous)out.redundant.push_back(id);
        }previous=end;previous_rank=current;}
        if(out.state=="conflict"){out.error="Vincoli in conflitto: ";for(const auto& id:out.conflicting)out.error+=id+" ";}
        Eigen::JacobiSVD<M> svd;M nullspace;
        if(j.rows()&&j.cols()){svd.compute(j,Eigen::ComputeFullV);nullspace=svd.matrixV().rightCols(out.degrees_of_freedom);}
        for(auto& p:out.sketch.points){int i=system.points.at(p.id);p.x=x[i];p.y=x[i+1];out.fixed_points[p.id]=out.degrees_of_freedom==0||(j.rows()&&nullspace.middleRows(i,2).norm()<1e-6);}
        for(auto& e:out.sketch.entities)if(system.radii.contains(e.id)){e.radius=x[system.radii.at(e.id)];if(e.radius<=0){out.state="conflict";out.error="Raggio non positivo: "+e.id;}}
        profiles(out);
    }catch(const std::exception& e){out.state="error";out.error=e.what();}
    return out;
}
nlohmann::json inspect(const SketchResult& r){auto j=doc::sketch_json(r.sketch);j["state"]=r.state;j["degrees_of_freedom"]=r.degrees_of_freedom;j["residual"]=r.residual;j["error"]=r.error;j["redundant"]=r.redundant;j["conflicting"]=r.conflicting;j["fixed_points"]=r.fixed_points;j["profiles"]=r.profiles;return j;}
SketchResult drag(const doc::Document& d,const doc::Sketch& s,const std::string& point,double x,double y){
    try {
        if(!std::isfinite(x)||!std::isfinite(y)||std::abs(x)>1e6||std::abs(y)>1e6)throw std::runtime_error("sketch.range: destinazione del drag non valida");
        auto base=solve(d,s);if(base.state=="error"||base.state=="conflict")return base;
        if(!base.fixed_points.contains(point))throw std::runtime_error("sketch.reference: punto inesistente: "+point);
        if(base.fixed_points.at(point))return base;
        auto t=base.sketch;t.constraints.push_back({"drag-target","fix",{point},{},{x,y}});auto result=solve(d,t);
        if(result.state!="conflict"&&result.state!="error") {result.sketch.constraints.pop_back();return solve(d,result.sketch);}
        // A point with one DOF cannot follow an arbitrary 2D target. Project the
        // pointer displacement into the null space, then restore the equations.
        // This follows circles/rails instead of rejecting an off-curve cursor.
        for(int iteration=0;iteration<32;++iteration){
            System system(d,base.sketch);const int index=system.points.at(point);const V current=system.initial;
            M j=system.jacobian(current);if(!j.rows()||!j.cols())break;
            Eigen::JacobiSVD<M> svd(j,Eigen::ComputeFullV);svd.setThreshold(1e-7);
            const int freedom=static_cast<int>(current.size()-svd.rank());if(!freedom)break;
            M nullspace=svd.matrixV().rightCols(freedom),a=nullspace.middleRows(index,2);
            P offset{x-current[index],y-current[index+1]};V step=nullspace*a.completeOrthogonalDecomposition().solve(offset);
            if(!step.allFinite()||step.norm()<1e-8)break;
            bool accepted=false;
            for(int trial=0;trial<12;++trial){
                V next=current+step;auto candidate=base.sketch;
                for(auto& p:candidate.points){int i=system.points.at(p.id);p.x=next[i];p.y=next[i+1];}
                for(auto& e:candidate.entities)if(system.radii.contains(e.id))e.radius=next[system.radii.at(e.id)];
                auto corrected=solve(d,candidate);
                if(corrected.state!="error"&&corrected.state!="conflict"){
                    auto p=std::find_if(corrected.sketch.points.begin(),corrected.sketch.points.end(),[&](const auto& q){return q.id==point;});
                    if(std::hypot(p->x-x,p->y-y)<offset.norm()-1e-10){base=std::move(corrected);accepted=true;break;}
                }
                step*=.5;
            }
            if(!accepted)break;
        }
        return base;
    }catch(const std::exception& e){SketchResult r;r.sketch=s;r.state="error";r.error=e.what();return r;}
}
}
