#pragma once
#include "ui/view_cube.hpp"
struct ImDrawList;
namespace sagomacad {
ui::CubeHit draw_view_cube_widget(ImDrawList* draw,ui::Camera& camera,ui::CubeInteraction& interaction,ui::CameraTransition& transition,
                           ui::CubePoint origin,const ui::Theme& theme,float scale,bool interactive);
}
