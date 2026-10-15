#include "ui/camera.hpp"
#include <cstdio>
#include <cstdlib>
#define require(condition) do { if (!(condition)) { std::fprintf(stderr,"failed: %s\n",#condition); std::exit(1); } } while (false)
#include <cmath>
int main() {
    using namespace sagomacad;
    ui::Camera c; ui::Viewport v{0,0,1000,700};
    auto center=c.project({0,0,0},v);
    require(center.visible && std::abs(center.x-500)<1e-8 && std::abs(center.y-350)<1e-8);
    const double before=c.distance;
    c.zoom(1,500,350,v); require(c.distance<before);
    c.orbit(100,40); require(std::abs(c.yaw+0.785398)>0.1);
    c.pan(100,50,700); require(std::abs(c.target.x)+std::abs(c.target.y)+std::abs(c.target.z)>0);
    c.look_from({0,0,1}); require(c.eye().z>c.target.z);
    require(std::abs(c.forward().x)<1e-10 && std::abs(c.forward().y)<1e-10);
    require(std::abs(c.right().x*c.right().x+c.right().y*c.right().y+c.right().z*c.right().z-1)<1e-10);
    require(std::abs(c.up().x*c.forward().x+c.up().y*c.forward().y+c.up().z*c.forward().z)<1e-10);
    c.fit({-20,-20,0},{20,20,20}); require(std::abs(c.distance-120)<1e-8);
    ui::Camera start,end;
    start.distance=900; end.look_from({0,0,1}); end.target={20,20,10}; end.distance=120;
    ui::CameraTransition transition;
    transition.begin(start,end,0.85); c=start;
    transition.advance(c,0); require(c.pitch==start.pitch && c.distance==start.distance);
    transition.advance(c,0.01); require(c.pitch>start.pitch && c.pitch-start.pitch<0.001);
    transition.advance(c,0.415);
    require(transition.active && std::abs(c.pitch-(start.pitch+end.pitch)*0.5)<1e-9);
    require(std::abs(c.target.x-10)<1e-9 && c.distance>120 && c.distance<900 && std::abs(c.roll)<1e-9);
    const auto midway=c;
    ui::CameraTransition divided; ui::Camera sampled=start;
    divided.begin(start,end,0.85);
    for (int i=0;i<85;++i) divided.advance(sampled,0.005);
    require(std::abs(sampled.pitch-c.pitch)<1e-9 && std::abs(sampled.distance-c.distance)<1e-9);
    transition.cancel(); transition.advance(c,1); require(c.pitch==midway.pitch && c.distance==midway.distance);
    end.look_from({1,0,0}); transition.begin(c,end,0.85); transition.advance(c,0);
    require(c.pitch==midway.pitch);
    transition.advance(c,1); require(!transition.active && c.pitch==end.pitch && c.yaw==end.yaw && c.distance==end.distance);
    start.yaw=3.12413936106985; end=start; end.yaw=-start.yaw;
    transition.begin(start,end,0.85); transition.advance(c,0.425);
    require(std::abs(c.yaw-3.141592653589793)<1e-8);
}
