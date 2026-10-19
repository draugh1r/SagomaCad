#pragma once
#include "ui/camera.hpp"
#include <array>
namespace sagomacad::ui {
struct GridLevel {double step=1,minor=0,major=0;};
struct GridScale {double step=1;std::array<GridLevel,3> levels;};
struct GridPlane {bool enabled=false;kernel::Vec3 origin{},u{1,0,0},v{0,1,0};};
GridScale grid_scale(double camera_distance,double viewport_height);
}
