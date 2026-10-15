#include "ui/view_cube.hpp"
#include <cmath>
#include <cstdio>
#include <stdexcept>
using namespace sagomacad;
#define require(condition) do { if (!(condition)) throw std::runtime_error(#condition); } while (false)
static bool near(double a,double b) { return std::abs(a-b)<1e-8; }
static void direction(const ui::Camera& camera,kernel::Vec3 expected) {
    const double n=std::sqrt(expected.x*expected.x+expected.y*expected.y+expected.z*expected.z);
    const auto f=camera.forward();
    require(near(f.x,-expected.x/n) && near(f.y,-expected.y/n) && near(f.z,-expected.z/n));
}
int main() {
    try {
        const auto& t=ui::light.cube;
        int faces=0,edges=0,corners=0;
        for (int x=-1;x<=1;++x) for (int y=-1;y<=1;++y) for (int z=-1;z<=1;++z) {
            const int count=(x!=0)+(y!=0)+(z!=0);
            if (!count) continue;
            const auto kind=count==1?ui::CubeKind::Face:count==2?ui::CubeKind::Edge:ui::CubeKind::Corner;
            ui::Camera camera; camera.look_from({double(x),double(y),double(z)});
            const auto layout=ui::cube_layout(camera,{0,0},t);
            const ui::CubePart* wanted=nullptr;
            for (const auto& part:layout.parts)
                if (part.hit.direction.x==x && part.hit.direction.y==y && part.hit.direction.z==z) wanted=&part;
            require(wanted && wanted->hit.kind==kind);
            ui::CubePoint center{};
            for (const auto p:wanted->polygon) { center.x+=p.x/wanted->polygon.size(); center.y+=p.y/wanted->polygon.size(); }
            const auto hit=ui::cube_hit(layout,center);
            require(hit.kind==kind && hit.direction.x==x && hit.direction.y==y && hit.direction.z==z);
            camera.home(); ui::cube_apply(camera,hit); direction(camera,{double(x),double(y),double(z)});
            faces+=count==1; edges+=count==2; corners+=count==3;
        }
        require(faces==6 && edges==12 && corners==8);
        ui::Camera top; top.look_from({0,0,1});
        const auto square=ui::cube_layout(top,{0,0},t);
        require(square.faces.size()==1 && square.faces[0].polygon.size()==4);
        for (const auto p:square.faces[0].polygon) {
            require(near(std::abs(p.x-t.center_x),square.factor));
            require(near(std::abs(p.y-t.center_y),square.factor));
        }
        ui::Camera camera; camera.look_from({0,0,1});
        ui::cube_apply(camera,{ui::CubeKind::Right,{}}); direction(camera,{1,0,0});
        ui::cube_apply(camera,{ui::CubeKind::Left,{}}); direction(camera,{0,0,1});
        camera.look_from({0,0,1});
        const auto initial_right=camera.right(),initial_up=camera.up();
        ui::cube_apply(camera,{ui::CubeKind::Clockwise,{}});
        direction(camera,{0,0,1}); require(near(camera.right().x,initial_up.x) && near(camera.right().y,initial_up.y));
        ui::cube_apply(camera,{ui::CubeKind::Counterclockwise,{}});
        require(near(camera.right().x,initial_right.x) && near(camera.right().y,initial_right.y));
        camera.roll_quarter(1);
        camera.orbit(0,20);
        require(near(camera.roll,0) && near(camera.right().z,0));
        for (int i=0;i<100;++i) {
            camera.orbit(i%2?25:-10,i%3?8:-15);
            require(near(camera.roll,0) && near(camera.right().z,0) && camera.up().z>0);
        }
        camera.look_from({0,0,1});
        camera.orbit(0,500); require(camera.up().z>=0 && near(camera.right().z,0));
        ui::cube_apply(camera,{ui::CubeKind::Home,{}}); direction(camera,{1,-1,1}); require(near(camera.roll,0));
        require(ui::cube_hit(ui::cube_layout(camera,{0,0},t),{-500,-500}).kind==ui::CubeKind::None);
        std::puts("View cube: 6 faces, 12 edges, 8 corners, Home and rotation verified");
    } catch (const std::exception& e) { std::fprintf(stderr,"%s\n",e.what()); return 1; }
}
