#include "ui/camera.hpp"
#include <algorithm>
#include <cmath>

namespace sagomacad::ui {
namespace {
kernel::Vec3 add(kernel::Vec3 a,kernel::Vec3 b) { return {a.x+b.x,a.y+b.y,a.z+b.z}; }
kernel::Vec3 mul(kernel::Vec3 a,double k) { return {a.x*k,a.y*k,a.z*k}; }
double dot(kernel::Vec3 a,kernel::Vec3 b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
kernel::Vec3 cross(kernel::Vec3 a,kernel::Vec3 b) { return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x}; }
kernel::Vec3 unit(kernel::Vec3 a) { const double n=std::sqrt(dot(a,a)); return n>1e-12?mul(a,1/n):kernel::Vec3{0,0,1}; }
}
kernel::Vec3 Camera::eye() const { return add(target,mul({std::cos(pitch)*std::cos(yaw),std::cos(pitch)*std::sin(yaw),std::sin(pitch)},distance)); }
kernel::Vec3 Camera::forward() const { return unit(add(target,mul(eye(),-1))); }
kernel::Vec3 Camera::right() const { return unit(cross(forward(),{0,0,1})); }
kernel::Vec3 Camera::up() const { return unit(cross(right(),forward())); }
Projection Camera::project(kernel::Vec3 point,Viewport v) const {
    const auto delta=add(point,mul(eye(),-1)); const double depth=dot(delta,forward());
    if (depth<=0.01 || v.width<=0 || v.height<=0) return {0,0,depth,false};
    const double focal=v.height*1.2071067811865475;
    return {v.x+v.width*0.5+dot(delta,right())*focal/depth,
            v.y+v.height*0.5-dot(delta,up())*focal/depth,depth,true};
}
void Camera::orbit(double dx,double dy) { yaw-=dx*0.008; pitch=std::clamp(pitch+dy*0.008,-1.55,1.55); }
void Camera::pan(double dx,double dy,double h) {
    if (h<=0) return;
    const double factor=distance/(h*1.2071067811865475);
    target=add(target,add(mul(right(),-dx*factor),mul(up(),dy*factor)));
}
void Camera::zoom(double wheel,double cx,double cy,Viewport v) {
    if (v.height<=0) return;
    const double old=distance;
    distance=std::clamp(distance*std::exp(-wheel*0.16),0.05,1e7);
    const double factor=(old-distance)/(v.height*1.2071067811865475);
    target=add(target,add(mul(right(),(cx-v.x-v.width*0.5)*factor),mul(up(),-(cy-v.y-v.height*0.5)*factor)));
}
void Camera::look_from(kernel::Vec3 direction) {
    const auto d=unit(direction); yaw=std::atan2(d.y,d.x); pitch=std::asin(std::clamp(d.z,-1.0,1.0));
    pitch=std::clamp(pitch,-1.55,1.55);
}
void Camera::fit(kernel::Vec3 lo,kernel::Vec3 hi) {
    target=mul(add(lo,hi),0.5);
    const double span=std::max({hi.x-lo.x,hi.y-lo.y,hi.z-lo.z,1.0});
    distance=span*3.0;
}
}
