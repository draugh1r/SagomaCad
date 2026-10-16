#include "solver/profile_regions.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>
namespace sagomacad::solver {
std::vector<ProfilePoint> sample_loop(const doc::Sketch& s,const std::vector<std::string>& ids) {
    std::vector<ProfilePoint> out;
    auto point=[&](const std::string& id){for(const auto& p:s.points)if(p.id==id)return ProfilePoint{p.x,p.y};throw std::runtime_error("Punto del profilo mancante");};
    for(const auto& id:ids) {
        auto it=std::find_if(s.entities.begin(),s.entities.end(),[&](const auto& e){return e.id==id;});
        if(it==s.entities.end())throw std::runtime_error("Entità del profilo mancante: "+id);
        const auto& e=*it; std::vector<ProfilePoint> segment;
        if(e.kind=="line")segment={point(e.points[0]),point(e.points[1])};
        else {
            auto c=point(e.points[0]); double a=0,b=2*std::numbers::pi;
            if(e.kind=="arc"){auto p=point(e.points[1]),q=point(e.points[2]);a=std::atan2(p[1]-c[1],p[0]-c[0]);b=std::atan2(q[1]-c[1],q[0]-c[0]);while(b<=a)b+=2*std::numbers::pi;}
            for(int i=0;i<=128;++i){double t=a+(b-a)*i/128;segment.push_back({c[0]+e.radius*std::cos(t),c[1]+e.radius*std::sin(t)});}
        }
        if(!out.empty()&&std::hypot(out.back()[0]-segment.front()[0],out.back()[1]-segment.front()[1])>1e-5)std::reverse(segment.begin(),segment.end());
        out.insert(out.end(),segment.begin(),segment.end());
    }
    return out;
}
bool contains(const std::vector<ProfilePoint>& p,double x,double y){
    bool inside=false;if(p.size()<3)return false;
    for(size_t i=0,j=p.size()-1;i<p.size();j=i++)if((p[i][1]>y)!=(p[j][1]>y)&&x<(p[j][0]-p[i][0])*(y-p[i][1])/(p[j][1]-p[i][1])+p[i][0])inside=!inside;
    return inside;
}
bool contains(const ProfileRegion& r,double x,double y){if(!contains(r.polygon,x,y))return false;for(const auto& h:r.hole_polygons)if(contains(h,x,y))return false;return true;}
std::vector<ProfileRegion> profile_regions(const doc::Sketch& s,const std::vector<std::vector<std::string>>& loops){
    std::vector<ProfileRegion> out;
    for(auto ids:loops){auto polygon=sample_loop(s,ids);auto stable=ids;std::sort(stable.begin(),stable.end());out.push_back({"region:"+nlohmann::json(stable).dump(),ids,{},{polygon},{}});}
    std::vector<double> areas(out.size());std::vector<int> parent(out.size(),-1);
    for(size_t i=0;i<out.size();++i){const auto& p=out[i].polygon;for(size_t k=0;k<p.size();++k){const auto& q=p[(k+1)%p.size()];areas[i]+=p[k][0]*q[1]-q[0]*p[k][1];}areas[i]=std::abs(areas[i]);}
    for(size_t i=0;i<out.size();++i)for(size_t j=0;j<out.size();++j)if(areas[j]>areas[i]&&!out[i].polygon.empty()&&contains(out[j].polygon,out[i].polygon[0][0],out[i].polygon[0][1])&&(parent[i]<0||areas[j]<areas[parent[i]]))parent[i]=static_cast<int>(j);
    for(size_t i=0;i<out.size();++i)if(parent[i]>=0){auto& r=out[parent[i]];r.holes.push_back(out[i].boundary);r.hole_polygons.push_back(out[i].polygon);}
    return out;
}
nlohmann::json regions_json(const std::vector<ProfileRegion>& regions){auto j=nlohmann::json::array();for(const auto& r:regions)j.push_back({{"id",r.id},{"boundary",r.boundary},{"holes",r.holes}});return j;}
}
