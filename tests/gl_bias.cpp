#include "shell.hpp"
#include "ui/render/gl_viewport.hpp"
#include <SDL3/SDL_opengl.h>
#include <iostream>
using namespace sagomacad;
int main(){
    Shell shell(640,400,1,true,true);if(!shell.valid())return 1;
    auto box=kernel::make_box(40,30,10);if(!box)return 2;auto surface=mesh::build(*box.value,.2);if(!surface)return 3;
    ui::GlViewport renderer;ui::Camera camera;camera.target={20,15,8};camera.distance=105;camera.yaw=-1.4;
    for(double pitch:{.001,.035,.6}){
        camera.pitch=pitch;if(!renderer.render({*surface.value},"box",camera,800,600,ui::theme(true)))return 4;
        std::vector<unsigned char> pixels(800*600*4);glBindTexture(GL_TEXTURE_2D,renderer.texture());glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
        int visible=0;
        for(int i=1;i<20;++i){auto q=camera.project({2.*i,0,10},{0,0,800,600});bool ink=false;
            for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx){int x=int(q.x)+dx,y=599-int(q.y)+dy;if(x<0||x>=800||y<0||y>=600)continue;const auto* p=&pixels[(y*800+x)*4];ink|=p[0]<150&&p[1]<160;}
            visible+=ink;
        }
        if(visible<18){std::cerr<<"Discontinuous edge at pitch "<<pitch<<": "<<visible;return 5;}
    }
    return 0;
}
