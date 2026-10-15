#include "view_cube_widget.hpp"
#include <imgui.h>
#include <algorithm>
#include <cmath>

namespace sagomacad {
namespace {
ImU32 color(ui::Color c) { return ImGui::ColorConvertFloat4ToU32({c.r,c.g,c.b,c.a}); }
ImVec2 point(ui::CubePoint p) { return {float(p.x),float(p.y)}; }
kernel::Vec3 add(kernel::Vec3 a,kernel::Vec3 b) { return {a.x+b.x,a.y+b.y,a.z+b.z}; }
kernel::Vec3 mul(kernel::Vec3 a,double n) { return {a.x*n,a.y*n,a.z*n}; }
bool same(ui::CubeHit a,ui::CubeHit b) {
    return a.kind==b.kind && a.direction.x==b.direction.x && a.direction.y==b.direction.y && a.direction.z==b.direction.z;
}
bool geometry(ui::CubeKind kind) { return kind==ui::CubeKind::Face || kind==ui::CubeKind::Edge || kind==ui::CubeKind::Corner; }
void face_text(ImDrawList* draw,const ui::CubePart& part,const ui::Camera& camera,const ui::CubeLayout& layout,const ui::ViewCubeTokens& tokens,float scale,ImU32 ink) {
    if (!part.label) return;
    auto* font=ImGui::GetFont();
    const float font_size=tokens.label_size*scale;
    const auto text_size=font->CalcTextSizeA(font_size,1000,0,part.label);
    const auto center=point(ui::cube_project(part.center,camera,layout,tokens,scale));
    const ImVec2 start={center.x-text_size.x*0.5f,center.y-text_size.y*0.5f};
    const double units=std::min(1/layout.factor,(2-2*tokens.label_inset)*0.9/std::max(1.0f,text_size.x));
    const int first=draw->VtxBuffer.Size;
    draw->AddText(font,font_size,start,ink,part.label);
    for (int i=first;i<draw->VtxBuffer.Size;++i) {
        auto& vertex=draw->VtxBuffer[i];
        const double x=(vertex.pos.x-center.x)*units,y=-(vertex.pos.y-center.y)*units;
        vertex.pos=point(ui::cube_project(add(part.center,add(mul(part.u,x),mul(part.v,y))),camera,layout,tokens,scale));
    }
}
void control_icon(ImDrawList* draw,const ui::CubeControl& control,const ui::Theme& theme,float scale,bool hovered) {
    const auto p=point(control.center);
    const float s=theme.cube.control_size*scale*0.5f,w=theme.cube.line_width*scale;
    const auto ink=color(hovered?theme.accent:theme.muted);
    using Kind=ui::CubeKind;
    if (control.kind==Kind::Home) {
        const ImVec2 house[]={{p.x-s*0.8f,p.y},{p.x,p.y-s*0.8f},{p.x+s*0.8f,p.y},
            {p.x+s*0.55f,p.y},{p.x+s*0.55f,p.y+s*0.7f},{p.x-s*0.55f,p.y+s*0.7f},{p.x-s*0.55f,p.y},{p.x-s*0.8f,p.y}};
        draw->AddPolyline(house,8,ink,0,w);
        draw->AddRectFilled({p.x-s*0.15f,p.y+s*0.2f},{p.x+s*0.15f,p.y+s*0.7f},ink);
    } else if (control.kind==Kind::Clockwise || control.kind==Kind::Counterclockwise) {
        const bool clockwise=control.kind==Kind::Clockwise;
        const double sign=clockwise?1:-1;
        constexpr double pi=3.141592653589793;
        const float r=s*0.72f;
        std::array<ImVec2,21> arc{};
        for (int i=0;i<=20;++i) {
            const double angle=(-pi*0.8+i*pi*1.4/20)*sign;
            arc[i]={p.x+float(std::cos(angle)*r),p.y+float(std::sin(angle)*r)};
        }
        draw->AddPolyline(arc.data(),int(arc.size()),ink,0,w);
        const auto end=arc.back(),previous=arc[arc.size()-2];
        const float dx=end.x-previous.x,dy=end.y-previous.y,n=std::max(0.001f,std::hypot(dx,dy));
        draw->AddTriangleFilled(end,{end.x-dx/n*s*0.65f-dy/n*s*0.35f,end.y-dy/n*s*0.65f+dx/n*s*0.35f},
            {end.x-dx/n*s*0.65f+dy/n*s*0.35f,end.y-dy/n*s*0.65f-dx/n*s*0.35f},ink);
    } else {
        ImVec2 direction{};
        if (control.kind==Kind::Up) direction={0,-1};
        if (control.kind==Kind::Down) direction={0,1};
        if (control.kind==Kind::Left) direction={-1,0};
        if (control.kind==Kind::Right) direction={1,0};
        const ImVec2 a={p.x+direction.x*s*0.45f,p.y+direction.y*s*0.45f};
        const ImVec2 b={p.x-direction.x*s*0.3f-direction.y*s*0.45f,p.y-direction.y*s*0.3f+direction.x*s*0.45f};
        const ImVec2 c={p.x-direction.x*s*0.3f+direction.y*s*0.45f,p.y-direction.y*s*0.3f-direction.x*s*0.45f};
        if (hovered) draw->AddTriangleFilled(a,b,c,ink);
        else draw->AddTriangle(a,b,c,ink,w);
    }
}
}
ui::CubeHit draw_view_cube_widget(ImDrawList* draw,ui::Camera& camera,ui::CubeInteraction& interaction,ui::CameraTransition& transition,
                           ui::CubePoint origin,const ui::Theme& theme,float scale,bool interactive) {
    auto layout=ui::cube_layout(camera,origin,theme.cube,scale);
    const auto& io=ImGui::GetIO();
    auto hit=interactive?ui::cube_hit(layout,{io.MousePos.x,io.MousePos.y}):ui::CubeHit{};
    if (interactive && hit.kind!=ui::CubeKind::None && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        transition.cancel();
        interaction.pressed=true; interaction.dragged=false; interaction.press=hit;
    }
    if (interaction.pressed && geometry(interaction.press.kind) && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        interaction.dragged=true;
        camera.orbit(io.MouseDelta.x,io.MouseDelta.y);
    }
    ui::CubeHit action;
    if (interaction.pressed && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        if (!interaction.dragged && same(hit,interaction.press)) {
            action=hit;
        }
        interaction={};
    }
    layout=ui::cube_layout(camera,origin,theme.cube,scale);
    hit=interactive?ui::cube_hit(layout,{io.MousePos.x,io.MousePos.y}):ui::CubeHit{};
    // Axes follow the camera and sit on the outside of the cube.
    const ImVec2 a={float(origin.x)+theme.cube.axis_origin[0]*scale,float(origin.y)+theme.cube.axis_origin[1]*scale};
    const auto r=camera.right(),u=camera.up();
    const std::array<ImVec2,3> directions{{{float(r.x),float(-u.x)},{float(r.y),float(-u.y)},{float(r.z),float(-u.z)}}};
    const std::array<ui::Color,3> colors{{theme.axis_x,theme.axis_y,theme.cube.axis_z}};
    const char* labels[]{"X","Y","Z"};
    for (int i=0;i<3;++i) {
        const auto d=directions[i];
        const ImVec2 b={a.x+d.x*theme.cube.axis_length*scale,a.y+d.y*theme.cube.axis_length*scale};
        draw->AddLine(a,b,color(colors[i]),theme.cube.line_width*scale);
        draw->AddText(ImGui::GetFont(),theme.cube.label_size*scale,
            {b.x+d.x*theme.cube.axis_label_gap*scale,b.y+d.y*theme.cube.axis_label_gap*scale},color(colors[i]),labels[i]);
    }
    for (const auto& part:layout.faces) {
        std::vector<ImVec2> polygon; for (const auto p:part.polygon) polygon.push_back(point(p));
        auto fill=theme.cube_face;
        const float light=float(0.65+part.light*0.35);
        fill.r*=light; fill.g*=light; fill.b*=light;
        draw->AddConvexPolyFilled(polygon.data(),int(polygon.size()),color(fill));
        draw->AddPolyline(polygon.data(),int(polygon.size()),color(theme.cube_edge),ImDrawFlags_Closed,theme.cube.line_width*scale);
        face_text(draw,part,camera,layout,theme.cube,scale,color(theme.text));
    }
    for (const auto& part:layout.parts) if (same(hit,part.hit)) {
        std::vector<ImVec2> polygon; for (const auto p:part.highlight) polygon.push_back(point(p));
        draw->AddConvexPolyFilled(polygon.data(),int(polygon.size()),color(theme.cube.hover));
    }
    for (const auto& control:layout.controls) control_icon(draw,control,theme,scale,hit.kind==control.kind);
    if (same(hit,interaction.hover)) interaction.hover_seconds+=io.DeltaTime;
    else { interaction.hover=hit; interaction.hover_seconds=0; }
    if (hit.kind!=ui::CubeKind::None && interaction.hover_seconds>=theme.cube.tooltip_delay) {
        const char* hint="Trascina per ruotare · clic per allineare";
        if (hit.kind==ui::CubeKind::Home) hint="Home · vista isometrica e adatta alla finestra";
        if (hit.kind==ui::CubeKind::Clockwise || hit.kind==ui::CubeKind::Counterclockwise) hint="Ruota la vista di 90°";
        if (hit.kind==ui::CubeKind::Up || hit.kind==ui::CubeKind::Down || hit.kind==ui::CubeKind::Left || hit.kind==ui::CubeKind::Right) hint="Passa alla faccia adiacente";
        if (!interaction.pressed) ImGui::SetTooltip("%s",hint);
    }
    return action;
}
}
