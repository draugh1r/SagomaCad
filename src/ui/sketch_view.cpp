#include "ui/sketch_view.hpp"
#include <cmath>
#include <limits>
namespace sagomacad::ui {
kernel::Vec3 sketch_world(const doc::Sketch& s,engine::SketchXY p){if(!s.support)return sketch_world(s.plane,p);return {s.origin[0]+p.x*s.u[0]+p.y*s.v[0],s.origin[1]+p.x*s.u[1]+p.y*s.v[1],s.origin[2]+p.x*s.u[2]+p.y*s.v[2]};}
bool sketch_cursor(const Camera& c,Viewport v,const doc::Sketch& s,double x,double y,engine::SketchXY& result){
    if(!s.support)return sketch_cursor(c,v,s.plane,x,y,result);if(v.height<=0)return false;auto eye=c.eye(),f=c.forward(),r=c.right(),u=c.up();double sx=(x-v.x-v.width/2)/(v.height*1.2071067811865475),sy=-(y-v.y-v.height/2)/(v.height*1.2071067811865475);
    kernel::Vec3 ray{f.x+sx*r.x+sy*u.x,f.y+sx*r.y+sy*u.y,f.z+sx*r.z+sy*u.z},n{s.u[1]*s.v[2]-s.u[2]*s.v[1],s.u[2]*s.v[0]-s.u[0]*s.v[2],s.u[0]*s.v[1]-s.u[1]*s.v[0]};double den=ray.x*n.x+ray.y*n.y+ray.z*n.z;if(std::abs(den)<1e-9)return false;double t=((s.origin[0]-eye.x)*n.x+(s.origin[1]-eye.y)*n.y+(s.origin[2]-eye.z)*n.z)/den;if(t<=0)return false;double a=eye.x+t*ray.x-s.origin[0],b=eye.y+t*ray.y-s.origin[1],d=eye.z+t*ray.z-s.origin[2];result={a*s.u[0]+b*s.u[1]+d*s.u[2],a*s.v[0]+b*s.v[1]+d*s.v[2]};return std::isfinite(result.x)&&std::isfinite(result.y);
}
Camera sketch_camera(const Camera& camera,const doc::Sketch& s){if(!s.support)return sketch_camera(camera,s.plane);auto c=camera;kernel::Vec3 n{s.u[1]*s.v[2]-s.u[2]*s.v[1],s.u[2]*s.v[0]-s.u[0]*s.v[2],s.u[0]*s.v[1]-s.u[1]*s.v[0]};double d=(c.target.x-s.origin[0])*n.x+(c.target.y-s.origin[1])*n.y+(c.target.z-s.origin[2])*n.z;c.target={c.target.x-d*n.x,c.target.y-d*n.y,c.target.z-d*n.z};c.look_from(n,{s.v[0],s.v[1],s.v[2]});return c;}
kernel::Vec3 sketch_world(std::string_view plane,engine::SketchXY p){return plane=="XZ"?kernel::Vec3{p.x,0,p.y}:plane=="YZ"?kernel::Vec3{0,p.x,p.y}:kernel::Vec3{p.x,p.y,0};}
bool sketch_cursor(const Camera& c,Viewport v,std::string_view plane,double x,double y,engine::SketchXY& result){
    if(v.height<=0)return false;auto eye=c.eye(),f=c.forward(),r=c.right(),u=c.up();double sx=(x-v.x-v.width/2)/(v.height*1.2071067811865475),sy=-(y-v.y-v.height/2)/(v.height*1.2071067811865475);
    kernel::Vec3 ray{f.x+sx*r.x+sy*u.x,f.y+sx*r.y+sy*u.y,f.z+sx*r.z+sy*u.z};
    double denominator=plane=="XZ"?ray.y:plane=="YZ"?ray.x:ray.z;
    if(std::abs(denominator)<1e-10)return false;double t=-(plane=="XZ"?eye.y:plane=="YZ"?eye.x:eye.z)/denominator;if(t<=0)return false;
    kernel::Vec3 p{eye.x+t*ray.x,eye.y+t*ray.y,eye.z+t*ray.z};result=plane=="XZ"?engine::SketchXY{p.x,p.z}:plane=="YZ"?engine::SketchXY{p.y,p.z}:engine::SketchXY{p.x,p.y};return std::isfinite(result.x)&&std::isfinite(result.y);
}
Camera sketch_camera(const Camera& camera,std::string_view plane){auto c=camera;c.target={};if(plane=="XZ")c.look_from({0,-1,0},{0,0,1});else if(plane=="YZ")c.look_from({1,0,0},{0,0,1});else c.look_from({0,0,1},{0,1,0});return c;}
std::string pick_sketch_plane(const Camera& c,Viewport v,double x,double y,double extent){
    std::string selected;double nearest=std::numeric_limits<double>::max();for(auto plane:{"XY","XZ","YZ"}){engine::SketchXY p;if(!sketch_cursor(c,v,plane,x,y,p)||p.x<0||p.y<0||p.x>extent||p.y>extent)continue;auto w=sketch_world(plane,p),e=c.eye();double dist=std::hypot(std::hypot(w.x-e.x,w.y-e.y),w.z-e.z);if(dist<nearest){nearest=dist;selected=plane;}}return selected;
}
}
