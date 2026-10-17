#include "shell.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <SDL3/SDL.h>
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#define require(c) do { if (!(c)) { std::fprintf(stderr,"line %d: %s\n",__LINE__,#c); std::exit(1); } } while(false)
using namespace sagomacad;
int main(int argc,char** argv) {
    const auto root=std::filesystem::absolute(argc>1?argv[1]:"layout-evidence");
    std::filesystem::create_directories(root);
    const auto settings=(root/"layout.json").string();
    std::filesystem::remove(settings);
    float browser=0,properties=0,timeline=0;
    {
        engine::Engine engine;
        Shell shell(1280,720,1,true,false);
        require(shell.valid()); shell.bind_engine(&engine); shell.set_settings_file(settings);
        shell.state().properties=true;
        auto frame=[&] { shell.render(); };
        for(int i=0;i<3;++i) frame();
        auto box=[&](const char* name) {
            const auto inspection=shell.inspect();
            for (const auto& r:inspection["shell_rects"]) if(r["id"]==name)
                return ui::LayoutRect{r["x"].get<float>(),r["y"].get<float>(),r["width"].get<float>(),r["height"].get<float>()};
            require(false); return ui::LayoutRect{};
        };
        auto mouse=[&](float x,float y,bool pressed) {
            ImGui::GetIO().AddMousePosEvent(x,y); ImGui::GetIO().AddMouseButtonEvent(0,pressed); frame();
        };
        auto drag=[&](const char* name,float dx,float dy) {
            const auto r=box(name); require(r.visible());
            const float x=r.x+r.width*0.5f,y=r.y+r.height*0.5f;
            mouse(x,y,false); mouse(x,y,true); mouse(x+dx,y+dy,true); mouse(x+dx,y+dy,false); frame();
        };
        auto snapshot=[&](const char* name) {
            ImGui::GetIO().AddMousePosEvent(-100,-100); frame();
            std::string error; require(shell.screenshot((root/name).string(),error));
        };
        snapshot("before-resize.png");
        drag("browser_separator",55,0); require(shell.state().browser_width==298);
        drag("properties_separator",-35,0); require(shell.state().properties_width==288);
        drag("timeline_separator",0,-44); require(shell.state().timeline_height==106);
        browser=shell.state().browser_width; properties=shell.state().properties_width; timeline=shell.state().timeline_height;
        require(std::filesystem::is_regular_file(settings));
        snapshot("after-resize.png");
        auto r=box("browser");
        const auto& t=ui::dark;
        const float x=r.x+r.width-t.layout.collapse_button+7,y=r.y+t.padding+7;
        mouse(x,y,false); mouse(x,y,true); mouse(x,y,false); frame();
        require(shell.state().browser_collapsed); require(box("browser").width==t.layout.collapsed_width);
        snapshot("browser-collapsed.png");
        r=box("browser");
        mouse(r.x+11,r.y+t.padding+7,false); mouse(r.x+11,r.y+t.padding+7,true); mouse(r.x+11,r.y+t.padding+7,false); frame();
        require(!shell.state().browser_collapsed); require(shell.state().browser_width==browser);
        r=box("properties");
        const float px=r.x+r.width-t.layout.collapse_button+7,py=r.y+t.padding+7;
        mouse(px,py,false); mouse(px,py,true); mouse(px,py,false); frame();
        require(shell.state().properties_collapsed); require(box("properties").width==t.layout.collapsed_width);
        r=box("properties");
        mouse(r.x+11,r.y+t.padding+7,false); mouse(r.x+11,r.y+t.padding+7,true); mouse(r.x+11,r.y+t.padding+7,false); frame();
        require(!shell.state().properties_collapsed); require(shell.state().properties_width==properties);
        // Switching to an empty ribbon tab must never use the previous tab's rectangles.
        mouse(110,49,false); mouse(110,49,true); mouse(110,49,false); frame();
        require(shell.state().selected_tab==2);
        mouse(42,49,false); mouse(42,49,true); mouse(42,49,false); frame();
        require(shell.state().selected_tab==0);
        snapshot("before-restart.png");
    }
    {
        Shell shell(1280,720,1,true,false);
        require(shell.valid()); shell.set_settings_file(settings);
        for(int i=0;i<3;++i) shell.render();
        require(shell.state().browser_width==browser&&shell.state().properties_width==properties&&shell.state().timeline_height==timeline);
        require(shell.state().properties&&!shell.state().browser_collapsed);
        std::string error; require(shell.screenshot((root/"after-restart.png").string(),error));
        auto separator=[&](const char* id) {
            const auto inspection=shell.inspect();
            for(const auto& r:inspection["shell_rects"]) if(r["id"]==id) return ImVec2{r["x"].get<float>()+r["width"].get<float>()*0.5f,r["y"].get<float>()+r["height"].get<float>()*0.5f};
            require(false); return ImVec2{};
        };
        auto click=[&](ImVec2 p,bool down) { ImGui::GetIO().AddMousePosEvent(p.x,p.y); ImGui::GetIO().AddMouseButtonEvent(0,down); shell.render(); };
        for(const char* id:{"browser_separator","properties_separator","timeline_separator"}) {
            const auto p=separator(id);
            click(p,false); click(p,true); click(p,false); click(p,true); click(p,false); shell.render();
        }
        require(shell.state().browser_width==ui::dark.sidebar_width-ui::dark.layout.panel_border);
        require(shell.state().properties_width==ui::dark.inspector_width-ui::dark.layout.panel_border);
        require(shell.state().timeline_height==ui::dark.timeline_height);
        int count=0; auto** windows=SDL_GetWindows(&count); require(count==1);
        require(SDL_SetWindowSize(windows[0],800,600)); SDL_free(windows);
        shell.poll(); for(int i=0;i<3;++i) shell.render();
        const auto state=shell.inspect();
        require(state["width"]==800&&state["height"]==600);
        const auto* window=ImGui::FindWindowByName("Viewport"); require(window);
        require(window->Size.x>=ui::dark.layout.viewport_min_width);
        require(shell.screenshot((root/"small-800x600.png").string(),error));
        std::ofstream(root/"inspect-800x600.json")<<state.dump(2);
        // Corrupt preference files leave a usable default UI.
    }
    std::ofstream(settings)<<"{ broken";
    { Shell shell(800,600,1,true,true); require(shell.valid()); shell.set_settings_file(settings); shell.render(); require(shell.state().browser_width==ui::light.sidebar_width-ui::light.layout.panel_border); }
    std::filesystem::remove(settings);
}
