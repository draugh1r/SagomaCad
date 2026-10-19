#include "shell.hpp"
#include "ui/render/gl_viewport.hpp"
#include <SDL3/SDL_opengl.h>
#include <iostream>
using namespace sagomacad;
int main(){
    Shell shell(640,400,1,true,true);if(!shell.valid())return 1;
    auto shape=kernel::make_box(20,20,20);if(!shape)return 2;auto surface=mesh::build(*shape.value,.2);if(!surface)return 3;
    ui::GlViewport renderer;ui::Camera camera;camera.target={10,10,0};camera.distance=80;camera.look_from({0,0,1});
    auto read=[&]{std::vector<unsigned char> p(600*400*4);glBindTexture(GL_TEXTURE_2D,renderer.texture());glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,p.data());return p;};
    auto theme=ui::theme(true);
    ui::GlViewport::Layer preview{*surface.value,{.9f,.15f,.12f,.3f},{0,0,0,0},1.2f,true,false};
    renderer.set_layers({preview});renderer.set_grid({false});if(!renderer.render({},"empty",camera,600,400,theme))return 4;auto plain=read();
    renderer.set_grid({true});if(!renderer.render({},"empty",camera,600,400,theme))return 5;auto grid=read();
    for(int x=2;x<20;x+=3)for(int y=2;y<20;y+=3){auto p=camera.project({double(x),double(y),20},{0,0,600,400});int at=((399-int(p.y))*600+int(p.x))*4;
        for(int c=0;c<3;++c)if(std::abs(int(plain[at+c])-int(grid[at+c]))>1){std::cerr<<"Grid visible through transparent preview";return 6;}}
    renderer.set_layers({});renderer.set_grid({false});if(!renderer.render({*surface.value},"box",camera,600,400,theme))return 7;plain=read();
    ui::GlViewport::Layer selection;selection.face={1,0,0,.5f};
    for(auto t:surface.value->geometry.triangles)if(t.normal.z<-.9)selection.surface.geometry.triangles.push_back(t);
    renderer.set_layers({selection});if(!renderer.render({*surface.value},"box",camera,600,400,theme))return 8;grid=read();
    auto center=camera.project({10,10,20},{0,0,600,400});int at=((399-int(center.y))*600+int(center.x))*4;
    for(int c=0;c<3;++c)if(plain[at+c]!=grid[at+c]){std::cerr<<"Hidden selected face leaks through solid";return 9;}
    selection.surface.geometry.triangles.clear();for(auto t:surface.value->geometry.triangles)if(t.normal.z>.9)selection.surface.geometry.triangles.push_back(t);
    renderer.set_layers({selection});if(!renderer.render({*surface.value},"box",camera,600,400,theme))return 10;grid=read();
    if(int(grid[at])-int(plain[at])<20){std::cerr<<"Visible selection missing";return 11;}
    return glGetError()==GL_NO_ERROR?0:12;
}
