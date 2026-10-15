#pragma once
#include "ui/camera.hpp"
#include "engine/tools/sketch_overlay.hpp"
namespace sagomacad::ui {
kernel::Vec3 sketch_world(std::string_view plane,engine::SketchXY point);
bool sketch_cursor(const Camera&,Viewport,std::string_view plane,double mouse_x,double mouse_y,engine::SketchXY&);
Camera sketch_camera(const Camera&,std::string_view plane);
std::string pick_sketch_plane(const Camera&,Viewport,double x,double y,double extent);
}
