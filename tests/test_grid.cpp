#include "shell.hpp"
#include "ui/render/grid.hpp"
#include "ui/render/gl_viewport.hpp"
#include <SDL3/SDL_opengl.h>
#include <cmath>
#include <iostream>
using namespace sagomacad;
int main(){
    for(double height:{200.,600.,1800.})for(double distance:{.05,1.,10.,140.,10000.,1e7}){
        auto grid=ui::grid_scale(distance,height);double pixels=grid.step*height*1.2071067811865475/distance;
        if(pixels<8-1e-8||pixels>40+1e-8)return 1;
        if(grid.levels[1].step!=grid.step*5)return 2;
    }
    for(int k=-6;k<7;++k){
        double step=std::pow(5.,k),distance=600*1.2071067811865475*step/40;
        auto a=ui::grid_scale(distance*(1-1e-7),600),b=ui::grid_scale(distance*(1+1e-7),600);
        for(int j=-1;j<4;++j){double world_step=step*std::pow(5.,j);double minor_a=0,minor_b=0,major_a=0,major_b=0;
            for(auto l:a.levels)if(std::abs(l.step-world_step)<world_step*1e-5){minor_a+=l.minor;major_a+=l.major;}
            for(auto l:b.levels)if(std::abs(l.step-world_step)<world_step*1e-5){minor_b+=l.minor;major_b+=l.major;}
            if(std::abs(minor_a-minor_b)>1e-5||std::abs(major_a-major_b)>1e-5){std::cerr<<"Discontinuous level";return 3;}
        }
    }
    Shell shell(640,400,1,true,true);if(!shell.valid())return 4;
    auto box=kernel::make_box(20,20,20);if(!box)return 5;auto surface=mesh::build(*box.value,.2);if(!surface)return 6;
    ui::GlViewport renderer;ui::Camera camera;camera.target={10,10,0};camera.distance=80;
    for(bool light:{false,true})for(double direction:{-1.,1.}){
        auto theme=ui::theme(light);camera.look_from({0,0,direction});
        std::vector<unsigned char> plain(600*400*4),grid(plain.size());
        for(bool enabled:{false,true}){
            renderer.set_grid({enabled});if(!renderer.render({*surface.value},"box",camera,600,400,theme))return 7;
            glBindTexture(GL_TEXTURE_2D,renderer.texture());glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,(enabled?grid:plain).data());
        }
        for(int x=2;x<20;x+=3)for(int y=2;y<20;y+=3){auto p=camera.project({double(x),double(y),direction>0?20.:0.},{0,0,600,400});int at=((399-int(p.y))*600+int(p.x))*4;
            for(int c=0;c<3;++c)if(std::abs(int(grid[at+c])-int(plain[at+c]))>1){std::cerr<<"Grid leaks through body";return 8;}}
        int changed=0;for(std::size_t i=0;i<grid.size();i+=4)if(std::abs(int(grid[i])-int(plain[i]))>3)++changed;
        if(changed<500){std::cerr<<"Grid missing";return 9;}
    }
    return 0;
}
