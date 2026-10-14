#include "shell.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <cstdio>
#include <cstdlib>
#define require(condition) do { if (!(condition)) { std::fprintf(stderr,"failed: %s\n",#condition); std::exit(1); } } while (false)

int main() {
    sagomacad::engine::Engine engine;
    const auto added=engine.execute("primitive.add",{{"kind","box"},{"dimensions",{{"width","40 mm"},{"depth","40 mm"},{"height","20 mm"}}}});
    require(added.ok);
    sagomacad::Shell shell(1440,900,1,true,true);
    require(shell.valid());
    shell.bind_engine(&engine);
    for (int frame=0;frame<3;++frame) shell.render();
    const auto initial_camera=shell.state().camera;
    auto middle=[&](float x,float y,bool pressed) {
        ImGui::GetIO().AddMousePosEvent(x,y);
        ImGui::GetIO().AddMouseButtonEvent(ImGuiMouseButton_Middle,pressed);
        shell.render();
    };
    middle(800,500,false); middle(800,500,true); middle(860,530,true); middle(860,530,false);
    const auto panned=shell.state().camera;
    require(panned.target.x!=initial_camera.target.x || panned.target.y!=initial_camera.target.y);
    require(panned.yaw==initial_camera.yaw && panned.pitch==initial_camera.pitch);
    ImGui::GetIO().AddKeyEvent(ImGuiMod_Shift,true); shell.render();
    middle(800,500,false); middle(800,500,true); middle(860,530,true); middle(860,530,false);
    const auto orbited=shell.state().camera;
    require(orbited.yaw!=panned.yaw && orbited.pitch!=panned.pitch);
    require(orbited.target.x==panned.target.x && orbited.target.y==panned.target.y && orbited.target.z==panned.target.z);
    ImGui::GetIO().AddKeyEvent(ImGuiMod_Shift,false); shell.render();
    shell.state().camera=initial_camera;
    auto step=[&](bool pressed) {
        ImGui::GetIO().AddMousePosEvent(45,885);
        ImGui::GetIO().AddMouseButtonEvent(0,pressed);
        shell.render();
    };
    step(false); step(true); step(false); step(true);
    require(shell.state().primitive_editor_open);
    require(shell.state().primitive_feature_id==added.value["feature_id"].get<std::string>());
    step(false);
    auto right=[&](float x,float y,bool pressed) {
        ImGui::GetIO().AddMousePosEvent(x,y);
        ImGui::GetIO().AddMouseButtonEvent(1,pressed);
        shell.render();
    };
    right(45,885,false); right(45,885,true); right(45,885,false);
    require(ImGui::IsPopupOpen(nullptr,ImGuiPopupFlags_AnyPopupId));
    ImGui::GetIO().AddMousePosEvent(500,500);
    ImGui::GetIO().AddMouseButtonEvent(0,true); shell.render();
    ImGui::GetIO().AddMouseButtonEvent(0,false); shell.render();
    shell.state().primitive_editor_open=false;
    shell.render();
    auto left=[&](float x,float y,bool pressed) {
        ImGui::GetIO().AddMousePosEvent(x,y);
        ImGui::GetIO().AddMouseButtonEvent(0,pressed);
        shell.render();
    };
    shell.state().camera.distance=500;
    left(915,800,false); left(915,800,true); left(915,800,false);
    require(shell.state().camera.distance<130);
    left(768,800,false); left(768,800,true); left(768,800,false);
    require(shell.state().navigation_mode==0);
    const auto target=shell.state().camera.target;
    left(840,500,false); left(840,500,true); left(920,520,true); left(920,520,false);
    require(shell.state().camera.target.x!=target.x || shell.state().camera.target.y!=target.y);
    const auto document_before_typing=engine.session().document;
    shell.state().palette_open=true;
    shell.state().palette_focus=true;
    shell.render(); shell.render();
    left(600,260,false); left(600,260,true); left(600,260,false);
    shell.render();
    require(ImGui::GetIO().WantTextInput);
    ImGui::GetIO().AddKeyEvent(ImGuiMod_Ctrl,true);
    ImGui::GetIO().AddKeyEvent(ImGuiKey_Z,true);
    shell.render();
    require(engine.session().document==document_before_typing);
    ImGui::GetIO().AddKeyEvent(ImGuiMod_Ctrl,false);
    ImGui::GetIO().AddKeyEvent(ImGuiKey_Z,false);
    shell.render();
    shell.state().palette_open=false;
    ImGui::GetIO().AddMousePosEvent(500,500);
    ImGui::GetIO().AddMouseButtonEvent(0,true); shell.render();
    ImGui::GetIO().AddMouseButtonEvent(0,false); shell.render();
    require(!ImGui::IsPopupOpen(nullptr,ImGuiPopupFlags_AnyPopupId));
    right(110,454,false); right(110,454,true); right(110,454,false);
    require(ImGui::IsPopupOpen(nullptr,ImGuiPopupFlags_AnyPopupId));
    left(500,500,true); left(500,500,false);
    shell.state().status_message="Operazione rifiutata: parametro non valido";
    shell.render(); shell.render();
    const auto* error_window=ImGui::FindWindowByName("Errore##status");
    require(error_window && error_window->WasActive && error_window->DrawList->VtxBuffer.Size>0);
    shell.state().status_message.clear();
    require(engine.execute("document.new").ok);
    shell.render(); shell.render();
    const auto* empty_view=ImGui::FindWindowByName("Viewport");
    require(empty_view);
    const std::vector<ImDrawVert> empty_before(empty_view->DrawList->VtxBuffer.begin(),empty_view->DrawList->VtxBuffer.end());
    middle(800,500,false); middle(800,500,true); middle(860,530,true); middle(860,530,false);
    const auto& empty_after=empty_view->DrawList->VtxBuffer;
    bool grid_moved=empty_before.size()!=static_cast<std::size_t>(empty_after.Size);
    for (int i=0;!grid_moved && i<empty_after.Size;++i)
        grid_moved=empty_before[i].pos.x!=empty_after[i].pos.x || empty_before[i].pos.y!=empty_after[i].pos.y;
    require(grid_moved);
}
