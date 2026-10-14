#include "shell.hpp"
#include "ui/view_cube.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <cmath>
#include <filesystem>
#include <cstdio>
#include <cstdlib>
using namespace sagomacad;
#define require(condition) do { if (!(condition)) { std::fprintf(stderr,"failed: %s\n",#condition); std::exit(1); } } while (false)
static bool near(double a,double b) { return std::abs(a-b)<1e-8; }
int main(int argc,char** argv) {
    for (int variant=0;variant<2;++variant) {
        const bool light=variant==0;
        engine::Engine engine;
        require(engine.execute("primitive.add",{{"kind","box"},{"dimensions",{{"width","40 mm"},{"depth","40 mm"},{"height","20 mm"}}}}).ok);
        Shell shell(light?1440:1280,light?900:720,1,true,light);
        require(shell.valid()); shell.bind_engine(&engine);
        for (int i=0;i<3;++i) shell.render();
        const auto& theme=ui::theme(light);
        auto layout=[&] {
            const auto* window=ImGui::FindWindowByName("Viewport"); require(window);
            return ui::cube_layout(shell.state().camera,
                {window->WorkRect.Max.x-theme.cube.widget_size-theme.padding,window->WorkRect.Min.y+theme.padding},theme.cube);
        };
        auto mouse=[&](ui::CubePoint p,bool down) {
            ImGui::GetIO().AddMousePosEvent(float(p.x),float(p.y));
            ImGui::GetIO().AddMouseButtonEvent(ImGuiMouseButton_Left,down); shell.render();
        };
        auto select=[&](ui::CubeKind kind,kernel::Vec3 direction={}) {
            const auto cube=layout(); ui::CubePoint target{}; bool found=false;
            for (const auto& control:cube.controls) if (control.kind==kind) { target=control.center; found=true; }
            for (const auto& part:cube.parts)
                if (part.hit.kind==kind && part.hit.direction.x==direction.x && part.hit.direction.y==direction.y && part.hit.direction.z==direction.z) {
                    for (const auto p:part.polygon) { target.x+=p.x/part.polygon.size(); target.y+=p.y/part.polygon.size(); }
                    found=true;
                }
            require(found); mouse(target,false); mouse(target,true); mouse(target,false); shell.render();
        };
        auto screenshot=[&](const char* name) {
            if (argc<2) return;
            const auto path=std::filesystem::path(argv[1])/(std::string("viewcube-")+name+(light?"-light-1440x900.png":"-dark-1280x720.png"));
            std::string error; require(shell.screenshot(path.string(),error));
        };
        ImGui::GetIO().AddMousePosEvent(500,500); shell.render(); screenshot("iso");
        select(ui::CubeKind::Face,{0,0,1});
        require(near(shell.state().camera.forward().z,-1));
        ImGui::GetIO().AddMousePosEvent(500,500); shell.render(); screenshot("top");
        select(ui::CubeKind::Clockwise); require(near(shell.state().camera.roll,1.5707963267948966));
        select(ui::CubeKind::Counterclockwise); require(near(shell.state().camera.roll,0));
        select(ui::CubeKind::Right); require(near(shell.state().camera.forward().x,-1));
        shell.state().camera.distance=900; shell.state().camera.target={200,200,200};
        select(ui::CubeKind::Home);
        require(shell.state().camera.distance<130 && near(shell.state().camera.target.x,20));
        select(ui::CubeKind::Edge,{0,-1,1});
        require(near(shell.state().camera.forward().x,0) && near(shell.state().camera.forward().y,std::sqrt(0.5)));
        select(ui::CubeKind::Home); select(ui::CubeKind::Corner,{1,-1,1});
        require(near(shell.state().camera.forward().x,-1/std::sqrt(3.0)));
        select(ui::CubeKind::Home);
        const auto cube=layout(); ui::CubePoint edge{};
        for (const auto& part:cube.parts) if (part.hit.kind==ui::CubeKind::Edge && part.hit.direction.y==-1 && part.hit.direction.z==1)
            for (const auto p:part.polygon) { edge.x+=p.x/part.polygon.size(); edge.y+=p.y/part.polygon.size(); }
        mouse(edge,false); shell.render(); screenshot("edge-hover");
        const auto before=shell.state().camera;
        mouse(edge,true); mouse({edge.x+25,edge.y+15},true); mouse({edge.x+25,edge.y+15},false);
        require(!near(shell.state().camera.yaw,before.yaw) && !near(shell.state().camera.pitch,before.pitch));
        require(shell.state().camera.target.x==before.target.x && !shell.state().cube_interaction.pressed);
        require(shell.open_feature_editor("primitive-1"));
        shell.render(); shell.render();
        const auto* editor=ImGui::FindWindowByName("Modifica feature###primitive_panel");
        require(editor && editor->Pos.y>=layout().origin.y+theme.cube.widget_size);
        ImGui::GetIO().AddMousePosEvent(500,500); shell.render(); screenshot("edit");
    }
}
