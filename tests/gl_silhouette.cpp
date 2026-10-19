#include "shell.hpp"
#include "ui/render/gl_viewport.hpp"
#include <SDL3/SDL_opengl.h>
#include <cmath>
#include <iostream>
using namespace sagomacad;
int main(){
    Shell shell(640,400,1,true,true);if(!shell.valid())return 1;
    auto shape=kernel::make_cylinder(8,20);if(!shape)return 2;
    auto surface=mesh::build(*shape.value,.08);if(!surface)return 3;
    int curved=0;
    for(const auto& t:surface.value->geometry.triangles)for(int i=0;i<3;++i){
        const auto n=t.normals[i],p=t.vertices[i];
        if(std::abs(std::hypot(n.x,n.y,n.z)-1)>1e-6)return 4;
        if(std::abs(n.z)<.01){if(n.x*p.x+n.y*p.y<7.9)return 5;++curved;}
    }
    if(curved<30)return 6;
    surface.value->geometry.edges.clear();
    ui::Camera camera;camera.target={0,0,10};camera.distance=55;camera.look_from({1,-1,.3});
    ui::GlViewport renderer;auto theme=ui::theme(true);
    std::vector<unsigned char> plain(600*400*4),outlined(plain.size());
    for(bool silhouettes:{false,true}){
        renderer.set_style({true,true,silhouettes});
        if(!renderer.render({*surface.value},"cylinder",camera,600,400,theme))return 7;
        glBindTexture(GL_TEXTURE_2D,renderer.texture());glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,(silhouettes?outlined:plain).data());
    }
    int contour=0;for(std::size_t i=0;i<plain.size();i+=4)if(int(plain[i])-int(outlined[i])>30)++contour;
    if(contour<40){std::cerr<<"No curved silhouette: "<<contour;return 8;}
    renderer.set_style({false,true,true});
    if(!renderer.render({*surface.value},"cylinder",camera,600,400,theme))return 9;
    glBindTexture(GL_TEXTURE_2D,renderer.texture());glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,plain.data());
    for(std::size_t i=0;i<plain.size();i+=4)if(plain[i]!=255)return 10;
    return 0;
}
