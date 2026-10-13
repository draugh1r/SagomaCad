#pragma once
#include "kernel/shape.hpp"

namespace sagomacad::ui {
struct Viewport { double x=0,y=0,width=1,height=1; };
struct Projection { double x=0,y=0,depth=0; bool visible=false; };
class Camera {
public:
    kernel::Vec3 target{};
    double yaw=-0.7853981633974483, pitch=0.6108652381980153, distance=140.0;
    kernel::Vec3 eye() const;
    kernel::Vec3 right() const;
    kernel::Vec3 up() const;
    kernel::Vec3 forward() const;
    Projection project(kernel::Vec3 point,Viewport viewport) const;
    void orbit(double dx,double dy);
    void pan(double dx,double dy,double viewport_height);
    void zoom(double wheel,double cursor_x,double cursor_y,Viewport viewport);
    void look_from(kernel::Vec3 direction);
    void fit(kernel::Vec3 minimum,kernel::Vec3 maximum);
};
}
