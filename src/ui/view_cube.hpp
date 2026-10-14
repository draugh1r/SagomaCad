#pragma once
#include "ui/camera.hpp"
#include "ui/theme.hpp"
#include <array>
#include <vector>

namespace sagomacad::ui {
struct CubePoint { double x=0,y=0; };
enum class CubeKind { None,Face,Edge,Corner,Home,Up,Down,Left,Right,Clockwise,Counterclockwise };
struct CubeHit {
    CubeKind kind=CubeKind::None;
    kernel::Vec3 direction{};
};
struct CubePart {
    CubeHit hit;
    std::vector<CubePoint> polygon;
    kernel::Vec3 center{},u{},v{};
    const char* label=nullptr;
    double light=0;
};
struct CubeControl { CubeKind kind; CubePoint center; };
struct CubeLayout {
    std::vector<CubePart> parts;
    std::vector<CubeControl> controls;
    CubePoint origin;
    double factor=1, control_radius=10;
    bool face_aligned=false;
};
struct CubeInteraction {
    bool pressed=false,dragged=false;
    CubeHit press;
    CubeHit hover;
    double hover_seconds=0;
};
CubePoint cube_project(kernel::Vec3 point,const Camera& camera,const CubeLayout& layout,const ViewCubeTokens& tokens,double scale=1);
CubeLayout cube_layout(const Camera& camera,CubePoint origin,const ViewCubeTokens& tokens,double scale=1);
CubeHit cube_hit(const CubeLayout& layout,CubePoint point);
void cube_apply(Camera& camera,CubeHit hit);
}
