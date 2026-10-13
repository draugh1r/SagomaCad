#include "shell.hpp"
#include <imgui.h>
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
    require(!ImGui::IsPopupOpen(nullptr,ImGuiPopupFlags_AnyPopupId));
    right(110,454,false); right(110,454,true); right(110,454,false);
    require(ImGui::IsPopupOpen(nullptr,ImGuiPopupFlags_AnyPopupId));
}
