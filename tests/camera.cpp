#include "ui/camera.hpp"
#include <stdexcept>
#define require(condition) do { if (!(condition)) throw std::runtime_error(#condition); } while (false)
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
    c.fit({-20,-20,0},{20,20,20}); require(std::abs(c.distance-120)<1e-8);
}
