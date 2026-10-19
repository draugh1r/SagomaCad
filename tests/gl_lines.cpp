#include "shell.hpp"
#include "ui/render/gl_viewport.hpp"
#include <SDL3/SDL_opengl.h>
#include <iostream>
#include <vector>
using namespace sagomacad;
int main(){
    Shell shell(640,400,1,true,true);if(!shell.valid())return 1;
    ui::GlViewport renderer;ui::Camera camera;camera.target={};camera.look_from({0,0,1});camera.distance=10;
    renderer.set_style({false,true,false});
    auto theme=ui::theme(true);theme.solid_edge={0,0,0,1};theme.viewport={1,1,1,1};
    mesh::Surface surface;surface.geometry.edges.push_back({{{-5,0,0},{5,0,0}},1});
    for(float thickness:{1.2f,3.f,5.f})for(int size:{200,400})for(double distance:{10.,20.}){
        camera.distance=distance;theme.render.edge_px=thickness;
        if(!renderer.render({surface},"line",camera,size,size,theme)){std::cerr<<renderer.error();return 2;}
        std::vector<unsigned char> pixels(size*size*4);glBindTexture(GL_TEXTURE_2D,renderer.texture());glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
        double coverage=0;for(int y=0;y<size;++y)coverage+=1-pixels[(y*size+size/2)*4]/255.;
        if(std::abs(coverage-thickness)>.6){std::cerr<<"Pixel width "<<coverage<<" expected "<<thickness;return 3;}
    }
    camera.distance=10;surface.geometry.edges[0].points={{-5,0,20},{5,0,20}};
    if(!renderer.render({surface},"behind",camera,200,200,theme))return 4;
    std::vector<unsigned char> pixels(200*200*4);glBindTexture(GL_TEXTURE_2D,renderer.texture());glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
    for(std::size_t i=0;i<pixels.size();i+=4)if(pixels[i]!=255)return 5;
    surface.geometry.edges[0].points={{-5,0,9.995},{5,0,0}};
    if(!renderer.render({surface},"crossing",camera,200,200,theme))return 6;
    glBindTexture(GL_TEXTURE_2D,renderer.texture());glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
    int ink=0;for(std::size_t i=0;i<pixels.size();i+=4)if(pixels[i]<200)++ink;
    if(ink<20||ink>2000){std::cerr<<"Near-plane clipping failed: "<<ink;return 7;}
    return glGetError()==GL_NO_ERROR?0:8;
}
