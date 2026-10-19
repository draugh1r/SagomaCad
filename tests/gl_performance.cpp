#include "shell.hpp"
#include "ui/render/gl_viewport.hpp"
#include <SDL3/SDL_opengl.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <numbers>
using namespace sagomacad;
int main(){
    Shell shell(1440,900,1,true,true);if(!shell.valid()){std::cerr<<shell.error();return 1;}
    mesh::Surface sphere;sphere.geometry.triangles.reserve(200000);
    auto point=[](int i,int j){double a=std::numbers::pi*i/250.,b=2*std::numbers::pi*j/400.;return kernel::Vec3{30*std::sin(a)*std::cos(b),30*std::sin(a)*std::sin(b),30*std::cos(a)};};
    for(int i=0;i<250;++i)for(int j=0;j<400;++j){auto a=point(i,j),b=point(i+1,j),c=point(i+1,j+1),d=point(i,j+1);
        for(auto vertices:{std::array{a,b,c},std::array{a,c,d}}){kernel::Triangle triangle;triangle.vertices=vertices;for(int k=0;k<3;++k){auto p=vertices[k];triangle.normals[k]={p.x/30,p.y/30,p.z/30};}triangle.normal=triangle.normals[0];sphere.geometry.triangles.push_back(triangle);}}
    std::vector<mesh::Surface> scene;scene.push_back(std::move(sphere));
    ui::GlViewport renderer;ui::Camera camera;camera.distance=110;renderer.set_grid({true});
    std::vector<double> milliseconds;
    for(int frame=0;frame<132;++frame){camera.yaw+=.035;camera.pitch=.4+.3*std::sin(frame*.05);const auto start=std::chrono::steady_clock::now();
        if(!renderer.render(scene,"200000-triangles",camera,1440,900,ui::theme(true))){std::cerr<<renderer.error();return 2;}glFinish();
        if(frame>=12)milliseconds.push_back(std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());}
    double total=0;for(double t:milliseconds)total+=t;std::sort(milliseconds.begin(),milliseconds.end());
    const double average=1000.*milliseconds.size()/total,minimum=1000./milliseconds.back(),p95=1000./milliseconds[std::size_t(milliseconds.size()*.95)];
    std::cout<<nlohmann::json{{"triangles",200000},{"frames",milliseconds.size()},{"size",{1440,900}},{"samples",renderer.samples()},{"uploads",renderer.uploads()},{"fps",average},{"minimum_fps",minimum},{"p95_fps",p95},{"gpu",reinterpret_cast<const char*>(glGetString(GL_RENDERER))},{"version",reinterpret_cast<const char*>(glGetString(GL_VERSION))}}.dump()<<'\n';
    if(renderer.uploads()!=1)return 3;
    return minimum>60?0:4;
}
