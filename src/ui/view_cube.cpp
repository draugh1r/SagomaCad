#include "ui/view_cube.hpp"
#include <algorithm>
#include <cmath>

namespace sagomacad::ui {
namespace {
using Vec=kernel::Vec3;
double dot(Vec a,Vec b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
Vec add(Vec a,Vec b) { return {a.x+b.x,a.y+b.y,a.z+b.z}; }
Vec mul(Vec a,double n) { return {a.x*n,a.y*n,a.z*n}; }
Vec axis(int index,double sign) { Vec v{}; if(index==0)v.x=sign; if(index==1)v.y=sign; if(index==2)v.z=sign; return v; }
bool contains(const std::vector<CubePoint>& polygon,CubePoint p) {
    bool positive=false,negative=false;
    for (std::size_t i=0;i<polygon.size();++i) {
        const auto a=polygon[i],b=polygon[(i+1)%polygon.size()];
        const double cross=(b.x-a.x)*(p.y-a.y)-(b.y-a.y)*(p.x-a.x);
        positive|=cross>1e-6; negative|=cross<-1e-6;
    }
    return !(positive && negative);
}
}
CubePoint cube_project(Vec point,const Camera& camera,const CubeLayout& layout,const ViewCubeTokens& t,double scale) {
    return {layout.origin.x+t.center_x*scale+dot(point,camera.right())*layout.factor,
            layout.origin.y+t.center_y*scale-dot(point,camera.up())*layout.factor};
}
CubeLayout cube_layout(const Camera& camera,CubePoint origin,const ViewCubeTokens& t,double scale) {
    CubeLayout result; result.origin=origin; result.factor=t.cube_size*t.projection_scale*scale; result.control_radius=t.control_size*scale*0.5;
    const Vec eye=mul(camera.forward(),-1);
    const double inset=1-t.hit_band;
    auto make_part=[&](CubeKind kind,Vec normal,const std::vector<Vec>& vertices,const char* label,Vec u,Vec v,double light) {
        CubePart item; item.hit={kind,normal}; item.label=label; item.u=u; item.v=v;
        item.light=light;
        for (const auto vertex:vertices) {
            item.polygon.push_back(cube_project(vertex,camera,result,t,scale));
            item.center=add(item.center,mul(vertex,1.0/vertices.size()));
        }
        return item;
    };
    for (int a=0;a<3;++a) for (const double s:{-1.0,1.0}) {
        const Vec n=axis(a,s);
        const double facing=dot(n,eye);
        if (facing<=1e-8) continue;
        Vec u,v; const char* label;
        if (a==0) { u={0,s,0}; v={0,0,1}; label=s>0?t.right:t.left; }
        else if (a==1) { u={-s,0,0}; v={0,0,1}; label=s<0?t.front:t.back; }
        else { u={1,0,0}; v={0,s,0}; label=s>0?t.top:t.bottom; }
        auto patch=[&](CubeKind kind,Vec direction,double x0,double x1,double y0,double y1) {
            std::vector<Vec> vertices;
            for (const auto [x,y]:std::array<std::array<double,2>,4>{{{x0,y0},{x1,y0},{x1,y1},{x0,y1}}})
                vertices.push_back(add(n,add(mul(u,x),mul(v,y))));
            return make_part(kind,direction,vertices,label,u,v,facing);
        };
        result.faces.push_back(patch(CubeKind::Face,n,-1,1,-1,1));
        result.parts.push_back(patch(CubeKind::Face,n,-inset,inset,-inset,inset));
        for (const double side:{-1.0,1.0}) {
            const double low=side<0?-1:inset,high=side<0?-inset:1;
            result.parts.push_back(patch(CubeKind::Edge,add(n,mul(u,side)),low,high,-inset,inset));
            result.parts.push_back(patch(CubeKind::Edge,add(n,mul(v,side)),-inset,inset,low,high));
        }
        for (const double x:{-1.0,1.0}) for (const double y:{-1.0,1.0})
            result.parts.push_back(patch(CubeKind::Corner,add(n,add(mul(u,x),mul(v,y))),
                x<0?-1:inset,x<0?-inset:1,y<0?-1:inset,y<0?-inset:1));
        if (dot(n,eye)>1-1e-8) result.face_aligned=true;
    }
    auto control=[&](CubeKind kind,int index) {
        const auto p=t.control_positions[index]; result.controls.push_back({kind,{origin.x+p[0]*scale,origin.y+p[1]*scale}});
    };
    control(CubeKind::Home,0);
    if (result.face_aligned) {
        control(CubeKind::Up,1); control(CubeKind::Down,2);
        control(CubeKind::Left,3); control(CubeKind::Right,4);
        control(CubeKind::Counterclockwise,6); control(CubeKind::Clockwise,5);
    }
    return result;
}
CubeHit cube_hit(const CubeLayout& layout,CubePoint point) {
    for (const auto& control:layout.controls)
        if (std::abs(control.center.x-point.x)<=layout.control_radius && std::abs(control.center.y-point.y)<=layout.control_radius)
            return {control.kind,{}};
    for (const auto kind:{CubeKind::Corner,CubeKind::Edge,CubeKind::Face})
        for (const auto& part:layout.parts) if (part.hit.kind==kind && contains(part.polygon,point)) return part.hit;
    return {};
}
void cube_apply(Camera& camera,CubeHit hit) {
    if (hit.kind==CubeKind::Home) { camera.home(); return; }
    if (hit.kind==CubeKind::Clockwise) { camera.roll_quarter(1); return; }
    if (hit.kind==CubeKind::Counterclockwise) { camera.roll_quarter(-1); return; }
    if (hit.kind==CubeKind::Face || hit.kind==CubeKind::Edge || hit.kind==CubeKind::Corner) { camera.look_from(hit.direction); return; }
    const auto direction=mul(camera.forward(),-1),up=camera.up(),right=camera.right();
    if (hit.kind==CubeKind::Up) camera.look_from(up,mul(direction,-1));
    if (hit.kind==CubeKind::Down) camera.look_from(mul(up,-1),direction);
    if (hit.kind==CubeKind::Right) camera.look_from(right,up);
    if (hit.kind==CubeKind::Left) camera.look_from(mul(right,-1),up);
}
}
