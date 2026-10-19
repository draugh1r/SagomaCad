#include "ui/render/grid.hpp"
#include <algorithm>
#include <cmath>
namespace sagomacad::ui {
GridScale grid_scale(double distance,double height){
    if(!std::isfinite(distance)||!std::isfinite(height)||distance<=0||height<=0)return {};
    const double pixels=height*1.2071067811865475/distance;
    const double step=std::pow(5.,std::floor(std::log(40./pixels)/std::log(5.)));
    const double phase=std::clamp(std::log(step*pixels/8.)/std::log(5.),0.,1.);
    const double t=std::clamp(phase*2.,0.,1.),alpha=t*t*(3-2*t);
    return {step,{{{step,alpha,0},{5*step,1-phase,phase},{25*step,0,1-alpha}}}};
}
}
