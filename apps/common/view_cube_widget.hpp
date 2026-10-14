#pragma once
#include "ui/view_cube.hpp"
struct ImDrawList;
namespace sagomacad {
bool draw_view_cube_widget(ImDrawList* draw,ui::Camera& camera,ui::CubeInteraction& interaction,
                           ui::CubePoint origin,const ui::Theme& theme,float scale,bool interactive);
}
