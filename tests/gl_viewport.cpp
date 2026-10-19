#include "shell.hpp"
#include "ui/render/gl_viewport.hpp"
#include <SDL3/SDL_opengl.h>
#include <iostream>
int main(){
    sagomacad::Shell shell(640,400,1,true,true);
    if(!shell.valid()){std::cerr<<shell.error();return 1;}
    sagomacad::ui::GlViewport renderer;
    auto box=sagomacad::kernel::make_box(20,20,20);
    if(!box)return 2;
    auto surface=sagomacad::mesh::build(*box.value,.2);
    if(!surface)return 3;
    std::vector<sagomacad::mesh::Surface> scene{*surface.value};
    sagomacad::ui::Camera camera;
    camera.fit({0,0,0},{20,20,20});
    auto theme=sagomacad::ui::theme(true);
    int viewport[4]{13,17,300,200};glViewport(viewport[0],viewport[1],viewport[2],viewport[3]);
    if(!renderer.render(scene,"a",camera,500,300,theme)){std::cerr<<renderer.error();return 4;}
    if(!renderer.texture()||renderer.uploads()!=1||renderer.samples()<0||renderer.samples()>4)return 5;
    int restored[4]{};glGetIntegerv(GL_VIEWPORT,restored);for(int i=0;i<4;++i)if(viewport[i]!=restored[i])return 6;
    std::vector<unsigned char> pixels(500*300*4);
    glBindTexture(GL_TEXTURE_2D,renderer.texture());glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
    int colored=0;for(std::size_t i=0;i<pixels.size();i+=4)if(pixels[i]<240||pixels[i+1]<240||pixels[i+2]<240)++colored;
    if(colored<1000){std::cerr<<"No shaded geometry in resolved texture";return 10;}
    camera.orbit(10,5);
    if(!renderer.render(scene,"a",camera,640,400,theme)||renderer.uploads()!=1)return 7;
    if(!renderer.render(scene,"metadata-change",camera,640,400,theme)||renderer.uploads()!=1)return 12;
    if(!renderer.render({},"b",camera,640,400,theme)||renderer.uploads()!=1)return 8;
    if(renderer.render(scene,"bad",camera,0,400,theme))return 9;
    if(glGetError()!=GL_NO_ERROR){std::cerr<<"OpenGL error";return 11;}
    return 0;
}

