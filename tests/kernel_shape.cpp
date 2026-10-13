#include "kernel/shape.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace sagomacad::kernel;
static void require(bool condition,const char* message) { if (!condition) throw std::runtime_error(message); }
static bool near(double actual,double expected) { return std::abs(actual-expected)<1e-7*expected; }
int main() {
    try {
        const auto box=make_box(2,3,4);
        require(static_cast<bool>(box),"box non creato");
        const auto box_props=mass_properties(*box.value);
        require(box_props && near(box_props.value->volume,24) && near(box_props.value->area,52),"GProp box");
        const auto cylinder=make_cylinder(2,5);
        require(static_cast<bool>(cylinder),"cilindro non creato");
        const auto cylinder_props=mass_properties(*cylinder.value);
        require(cylinder_props && near(cylinder_props.value->volume,20*3.14159265358979323846),"GProp cilindro");
        const auto sphere=make_sphere(3);
        require(static_cast<bool>(sphere),"sfera non creata");
        const auto sphere_props=mass_properties(*sphere.value);
        require(sphere_props && near(sphere_props.value->volume,36*3.14159265358979323846),"GProp sfera");
        require(!make_box(0,1,1) && !make_cylinder(std::numeric_limits<double>::quiet_NaN(),1) &&
                !make_sphere(-1) && !mass_properties(Shape{}),"input invalidi accettati");
        std::cout<<"Kernel: volume e area GProp verificati\n";
        return 0;
    } catch (const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
