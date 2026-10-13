#include "shell.hpp"
#include "ui/theme.hpp"
#include "ui/toolbar_catalog.hpp"
#include "ui/parameter_editor.hpp"
#include "ui/feature_editor.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>
#include <lunasvg.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <cfloat>
#include <cctype>
#include <vector>
#include <cstring>
#include <array>
#include <bit>

namespace sagomacad {
namespace {
ImVec4 vec(ui::Color c) { return {c.r,c.g,c.b,c.a}; }
ImU32 packed(ui::Color c) { return ImGui::ColorConvertFloat4ToU32(vec(c)); }
void style(const ui::Theme& t, int scale) {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = t.rounding * scale;
    s.FrameRounding = t.rounding * scale;
    s.WindowPadding = {t.padding*scale,t.padding*scale};
    s.ItemSpacing = {t.spacing*scale,t.spacing*scale};
    auto& c = s.Colors;
    c[ImGuiCol_WindowBg] = vec(t.panel); c[ImGuiCol_ChildBg] = vec(t.panel);
    c[ImGuiCol_PopupBg] = vec(t.panel); c[ImGuiCol_Text] = vec(t.text);
    c[ImGuiCol_TextDisabled] = vec(t.muted); c[ImGuiCol_Border] = vec(t.border);
    c[ImGuiCol_FrameBg] = vec(t.panel_alt); c[ImGuiCol_FrameBgHovered] = vec(t.border);
    c[ImGuiCol_FrameBgActive] = vec(t.accent); c[ImGuiCol_TitleBg] = vec(t.panel);
    c[ImGuiCol_TitleBgActive] = vec(t.panel); c[ImGuiCol_MenuBarBg] = vec(t.panel);
    c[ImGuiCol_Button] = vec(t.panel_alt); c[ImGuiCol_ButtonHovered] = vec(t.border);
    c[ImGuiCol_ButtonActive] = vec(t.accent); c[ImGuiCol_Header] = vec(t.panel_alt);
    c[ImGuiCol_HeaderHovered] = vec(t.border); c[ImGuiCol_HeaderActive] = vec(t.accent);
    c[ImGuiCol_Tab] = vec(t.panel); c[ImGuiCol_TabHovered] = vec(t.border);
    c[ImGuiCol_TabSelected] = vec(t.panel_alt); c[ImGuiCol_DockingPreview] = vec(t.accent);
    c[ImGuiCol_TabDimmed] = vec(t.panel); c[ImGuiCol_TabDimmedSelected] = vec(t.panel_alt);
    c[ImGuiCol_TabSelectedOverline] = vec(t.accent);
    c[ImGuiCol_TabDimmedSelectedOverline] = vec(t.accent);
    c[ImGuiCol_DockingEmptyBg] = vec(t.background);
    c[ImGuiCol_Separator] = vec(t.border); c[ImGuiCol_ResizeGrip] = vec(t.border);
    c[ImGuiCol_TableHeaderBg] = vec(t.table_header);
    c[ImGuiCol_TableRowBg] = vec(t.panel);
    c[ImGuiCol_TableRowBgAlt] = vec(t.panel_alt);
}
void panel(const char* name, const char* label, const ui::Theme& t, auto contents, ImGuiWindowFlags extra = 0) {
    constexpr auto fixed = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
    if (ImGui::Begin(name, nullptr, fixed | extra)) {
        if (label) { ImGui::TextColored(vec(t.muted), "%s", label); ImGui::Separator(); }
        contents();
    }
    ImGui::End();
}
void icon(ImDrawList* dl, unsigned int texture, ImVec2 a, float size, ImU32 color) {
    if (texture) dl->AddImage(static_cast<ImTextureID>(texture),a,{a.x+size,a.y+size},{0,0},{1,1},color);
}
void centered_text(ImDrawList* dl, ImVec2 center, std::string_view label, ImU32 color, float wrap=0, float font_size=0) {
    const std::string value(label);
    if (font_size<=0) font_size=ImGui::GetFontSize();
    const ImVec2 size=ImGui::GetFont()->CalcTextSizeA(font_size,FLT_MAX,wrap,value.c_str());
    dl->AddText(ImGui::GetFont(),font_size,{center.x-size.x*0.5f,center.y},color,
        value.c_str(),nullptr,wrap);
}
ui::Color shaded(ui::Color base,double light) {
    const float intensity=static_cast<float>(std::clamp(0.62+0.38*light,0.27,1.12));
    return {std::min(1.0f,base.r*intensity),std::min(1.0f,base.g*intensity),std::min(1.0f,base.b*intensity),base.a};
}
bool inside_triangle(ImVec2 p,ImVec2 a,ImVec2 b,ImVec2 c) {
    auto sign=[](ImVec2 q,ImVec2 u,ImVec2 v) { return (q.x-v.x)*(u.y-v.y)-(u.x-v.x)*(q.y-v.y); };
    const bool n1=sign(p,a,b)<0,n2=sign(p,b,c)<0,n3=sign(p,c,a)<0;
    return n1==n2 && n2==n3;
}
double distance2(ImVec2 a,ImVec2 b) { return (a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y); }
struct DrawTriangle { ImVec2 a,b,c; double depth; ui::Color color; };
void draw_surface(ImDrawList* dl,const mesh::Surface& surface,const ui::Camera& camera,ui::Viewport viewport,const ui::Theme& theme,float scale) {
    std::vector<DrawTriangle> triangles;
    triangles.reserve(surface.geometry.triangles.size());
    for (const auto& tri:surface.geometry.triangles) {
        const auto a=camera.project(tri.vertices[0],viewport),b=camera.project(tri.vertices[1],viewport),c=camera.project(tri.vertices[2],viewport);
        if (!a.visible || !b.visible || !c.visible) continue;
        const auto& n=tri.normal;
        const double light=std::max(0.0,n.x*0.42+n.y*(-0.35)+n.z*0.84);
        triangles.push_back({{float(a.x),float(a.y)},{float(b.x),float(b.y)},{float(c.x),float(c.y)},(a.depth+b.depth+c.depth)/3.0,shaded(theme.solid,light)});
    }
    std::sort(triangles.begin(),triangles.end(),[](const auto& a,const auto& b){return a.depth>b.depth;});
    for (const auto& tri:triangles) dl->AddTriangleFilled(tri.a,tri.b,tri.c,packed(tri.color));
    for (const auto& edge:surface.geometry.edges) for (std::size_t i=1;i<edge.points.size();++i) {
        const auto p=camera.project(edge.points[i-1],viewport),q=camera.project(edge.points[i],viewport);
        if (!p.visible || !q.visible) continue;
        const int segments=edge.points.size()==2?20:1;
        for (int s=0;s<segments;++s) {
            const double t0=double(s)/segments,t1=double(s+1)/segments,tm=(t0+t1)*0.5;
            const ImVec2 midpoint={float(p.x+(q.x-p.x)*tm),float(p.y+(q.y-p.y)*tm)};
            const double depth=p.depth+(q.depth-p.depth)*tm;
            bool occluded=false;
            for (const auto& tri:triangles) if (tri.depth<depth-0.5 && inside_triangle(midpoint,tri.a,tri.b,tri.c)) { occluded=true; break; }
            if (!occluded) dl->AddLine({float(p.x+(q.x-p.x)*t0),float(p.y+(q.y-p.y)*t0)},
                                       {float(p.x+(q.x-p.x)*t1),float(p.y+(q.y-p.y)*t1)},packed(theme.solid_edge),1.15f*scale);
        }
    }
}
void draw_view_cube(ImDrawList* dl,ui::Camera& camera,ImVec2 origin,float size,const ui::Theme& theme,bool clicked,ImVec2 mouse) {
    const ImVec2 center={origin.x+size*0.5f,origin.y+size*0.5f};
    std::array<kernel::Vec3,8> points{};
    std::array<ImVec2,8> screen{};
    for (int i=0;i<8;++i) {
        points[i]={i&1?1.0:-1.0,i&2?1.0:-1.0,i&4?1.0:-1.0};
        const auto& q=points[i];
        screen[i]={center.x+float((q.x*camera.right().x+q.y*camera.right().y+q.z*camera.right().z)*size*0.26),
                   center.y-float((q.x*camera.up().x+q.y*camera.up().y+q.z*camera.up().z)*size*0.26)};
    }
    struct Face { std::array<int,4> corners; kernel::Vec3 normal; double depth; };
    std::array<Face,6> faces{{{{0,2,6,4},{-1,0,0},0},{{1,3,7,5},{1,0,0},0},
        {{0,1,5,4},{0,-1,0},0},{{2,3,7,6},{0,1,0},0},{{0,1,3,2},{0,0,-1},0},{{4,5,7,6},{0,0,1},0}}};
    const auto eye=camera.eye(); const kernel::Vec3 direction{eye.x-camera.target.x,eye.y-camera.target.y,eye.z-camera.target.z};
    for (auto& f:faces) f.depth=f.normal.x*direction.x+f.normal.y*direction.y+f.normal.z*direction.z;
    std::sort(faces.begin(),faces.end(),[](const auto& a,const auto& b){return a.depth<b.depth;});
    kernel::Vec3 selected{}; bool has_selection=false;
    for (const auto& f:faces) {
        if (f.depth<=0) continue;
        const auto a=screen[f.corners[0]],b=screen[f.corners[1]],c=screen[f.corners[2]],d=screen[f.corners[3]];
        dl->AddQuadFilled(a,b,c,d,packed(shaded(theme.cube_face,0.4+f.depth/std::max(camera.distance,1.0))));
        dl->AddQuad(a,b,c,d,packed(theme.cube_edge),1.0f);
        if (clicked && (inside_triangle(mouse,a,b,c)||inside_triangle(mouse,a,c,d))) { selected=f.normal; has_selection=true; }
    }
    if (clicked && has_selection) {
        // Corners and edges take precedence over the containing face.
        double best=size*size*0.015;
        for (int i=0;i<8;++i) { const double d=distance2(mouse,screen[i]); if (d<best) { best=d; selected=points[i]; } }
        if (best==size*size*0.015) for (int i=0;i<8;++i) for (int j=i+1;j<8;++j) {
            if (std::popcount(static_cast<unsigned int>(i^j))!=1) continue;
            const ImVec2 mid={(screen[i].x+screen[j].x)*0.5f,(screen[i].y+screen[j].y)*0.5f};
            const double d=distance2(mouse,mid);
            if (d<best) { best=d; selected={(points[i].x+points[j].x)*0.5,(points[i].y+points[j].y)*0.5,(points[i].z+points[j].z)*0.5}; }
        }
        camera.look_from(selected);
    }
    dl->AddText({origin.x,origin.y+size-12},packed(theme.muted),"VIEW");
}
}

Shell::Shell(int width, int height, int scale, bool hidden, bool light) : scale_(scale) {
    state_.width = width; state_.height = height; state_.theme = light ? "light" : "dark";
    if (!SDL_Init(SDL_INIT_VIDEO)) { error_ = SDL_GetError(); return; }
#ifdef __APPLE__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    const auto flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | (hidden ? SDL_WINDOW_HIDDEN : 0);
    window_ = SDL_CreateWindow("SagomaCad", width*scale, height*scale, flags);
    if (!window_) { error_ = SDL_GetError(); return; }
    context_ = SDL_GL_CreateContext(window_);
    if (!context_) { error_ = SDL_GetError(); SDL_DestroyWindow(window_); window_=nullptr; return; }
    SDL_GL_MakeCurrent(window_, context_);
    SDL_GL_SetSwapInterval(0);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename = nullptr;
    ImFontConfig font;
    font.SizePixels = 15.0f * scale;
    const std::string font_path=std::string(SAGOMACAD_ASSET_DIR)+"/fonts/Inter.ttf";
    static constexpr ImWchar latin_ranges[] = {0x0020,0x024F,0};
    if (!io.Fonts->AddFontFromFileTTF(font_path.c_str(),font.SizePixels,nullptr,latin_ranges)) {
        error_ = "Cannot load Inter font"; return;
    }
    style(ui::theme(light), scale);
    const char* glsl =
#ifdef __APPLE__
        "#version 410 core";
#else
        "#version 330 core";
#endif
    if (!ImGui_ImplSDL3_InitForOpenGL(window_, context_) || !ImGui_ImplOpenGL3_Init(glsl)) {
        error_ = "ImGui backend initialization failed"; return;
    }
    imgui_ready_ = true;
}
Shell::~Shell() {
    if (imgui_ready_) { ImGui_ImplOpenGL3_Shutdown(); ImGui_ImplSDL3_Shutdown(); }
    for (const auto& [_,texture] : icon_textures_) glDeleteTextures(1,&texture);
    if (ImGui::GetCurrentContext()) ImGui::DestroyContext();
    if (context_) SDL_GL_DestroyContext(context_);
    if (window_) SDL_DestroyWindow(window_);
    SDL_Quit();
}
unsigned int Shell::icon_texture(std::string_view name) {
    const std::string key(name);
    if (const auto it=icon_textures_.find(key);it!=icon_textures_.end()) return it->second;
    const auto path=std::string(SAGOMACAD_ASSET_DIR)+"/icons/lucide/"+key+".svg";
    auto doc=lunasvg::Document::loadFromFile(path);
    if (!doc) { icon_textures_[key]=0; return 0; }
    auto bitmap=doc->renderToBitmap(48,48);
    bitmap.convertToRGBA();
    auto* data=bitmap.data();
    for (int y=0;y<bitmap.height();++y) for (int x=0;x<bitmap.width();++x) {
        auto* pixel=data+y*bitmap.stride()+x*4;
        pixel[0]=pixel[1]=pixel[2]=255;
    }
    unsigned int texture=0;
    glGenTextures(1,&texture);
    glBindTexture(GL_TEXTURE_2D,texture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,bitmap.width(),bitmap.height(),0,GL_RGBA,GL_UNSIGNED_BYTE,data);
    glBindTexture(GL_TEXTURE_2D,0);
    icon_textures_[key]=texture;
    return texture;
}
bool Shell::poll() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);
        if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) return false;
        if (event.type == SDL_EVENT_WINDOW_RESIZED) {
            int w=0,h=0; SDL_GetWindowSize(window_, &w, &h);
            state_.width = w/scale_; state_.height = h/scale_;
        }
    }
    return true;
}
void Shell::draw() {
    const auto& t = ui::theme(state_.theme == "light");
    const float z = static_cast<float>(scale_);
    const ImVec2 screen = ImGui::GetIO().DisplaySize;
    const auto& input=ImGui::GetIO();
    if ((input.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_K)) ||
        (!input.KeyCtrl && !input.WantTextInput && ImGui::IsKeyPressed(ImGuiKey_S))) {
        state_.palette_open=true;
        state_.palette_focus=true;
    }
    if (input.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z)) invoke_command("edit.undo");
    if (input.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y)) invoke_command("edit.redo");
    if (input.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_N)) invoke_command("document.new");
    if (input.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_O)) invoke_command("document.open");
    if (input.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S))
        invoke_command(input.KeyShift?"document.saveAs":"document.save");
    const ImGuiID dock_id=ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
    static bool dock_initialized=false;
    if (!dock_initialized) {
        ImGui::DockBuilderRemoveNode(dock_id);
        ImGui::DockBuilderAddNode(dock_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dock_id,screen);
        ImGuiID middle=dock_id, header=0, timeline=0, browser=0, properties=0;
        ImGui::DockBuilderSplitNode(middle,ImGuiDir_Up,t.toolbar_height*z/screen.y,&header,&middle);
        const float timeline_height=glyph_sample_?t.timeline_glyph_height:t.timeline_height;
        ImGui::DockBuilderSplitNode(middle,ImGuiDir_Down,timeline_height*z/(screen.y-t.toolbar_height*z),&timeline,&middle);
        ImGui::DockBuilderSplitNode(middle,ImGuiDir_Left,t.sidebar_width*z/screen.x,&browser,&middle);
        if (state_.properties)
            ImGui::DockBuilderSplitNode(middle,ImGuiDir_Right,t.inspector_width*z/(screen.x-t.sidebar_width*z),&properties,&middle);
        std::vector<ImGuiID> nodes={header,timeline,browser,middle};
        if (properties) nodes.push_back(properties);
        for (ImGuiID node : nodes)
            ImGui::DockBuilderGetNode(node)->LocalFlags |= ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_NoResize;
        ImGui::DockBuilderDockWindow("SagomaCad  |  Solido",header);
        ImGui::DockBuilderDockWindow("Browser",browser);
        if (properties) ImGui::DockBuilderDockWindow("Proprietà",properties);
        ImGui::DockBuilderDockWindow("Timeline",timeline);
        ImGui::DockBuilderDockWindow("Viewport",middle);
        ImGui::DockBuilderFinish(dock_id);
        dock_initialized=true;
    }
    panel("SagomaCad  |  Solido", nullptr, t, [&] {
        ImDrawList* dl=ImGui::GetWindowDrawList();
        const ImVec2 p=ImGui::GetCursorScreenPos();
        const float width=ImGui::GetContentRegionAvail().x;
        auto command_button=[&](const ui::ToolbarCommand& command,ImVec2 a,ImVec2 size,bool show_label) {
            const auto status=ui::command_availability(command.id,engine_);
            const bool available=status.enabled;
            const std::string id(command.id);
            ImGui::SetCursorScreenPos(a);
            ImGui::PushID(id.c_str());
            ImGui::BeginDisabled(!available);
            const bool clicked=ImGui::InvisibleButton("command",size);
            ImGui::EndDisabled();
            const bool hovered=ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled);
            if (!available && hovered) ImGui::SetTooltip("%s",status.reason.c_str());
            if (clicked && available) invoke_command(command.id);
            if (hovered) dl->AddRectFilled(a,{a.x+size.x,a.y+size.y},packed(t.panel_alt),t.rounding*z);
            const float icon_size=(show_label?21.0f:17.0f)*z;
            icon(dl,icon_texture(command.icon),{a.x+(size.x-icon_size)*0.5f,a.y+2*z},icon_size,
                 packed(available?t.text:t.muted));
            if (show_label) centered_text(dl,{a.x+size.x*0.5f,a.y+26*z},command.label,
                packed(available?t.text:t.muted),size.x-4*z,12.5f*z);
            ImGui::PopID();
        };
        auto popup_command=[&](const ui::ToolbarCommand& command,float item_width) {
            const auto status=ui::command_availability(command.id,engine_);
            const bool available=status.enabled;
            ImDrawList* popup_dl=ImGui::GetWindowDrawList();
            ImGui::PushID(command.id.data());
            ImGui::BeginDisabled(!available);
            const bool clicked=ImGui::InvisibleButton("item",{item_width*z,27*z});
            ImGui::EndDisabled();
            const ImVec2 q=ImGui::GetItemRectMin();
            const bool hovered=ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled);
            if (hovered) popup_dl->AddRectFilled(q,{q.x+item_width*z,q.y+27*z},packed(t.panel_alt));
            icon(popup_dl,icon_texture(command.icon),{q.x+4*z,q.y+4*z},18*z,packed(available?t.text:t.muted));
            popup_dl->AddText({q.x+30*z,q.y+5*z},packed(available?t.text:t.muted),std::string(command.label).c_str());
            if (!available && hovered) ImGui::SetTooltip("%s",status.reason.c_str());
            if (clicked && available) { invoke_command(command.id); ImGui::CloseCurrentPopup(); }
            ImGui::PopID();
        };
        dl->AddText(p,packed(t.accent),"SagomaCad");
        const std::string document_name=engine_?engine_->session().document.name:"Nuovo documento";
        dl->AddText({p.x+126*z,p.y},packed(t.text),document_name.c_str());
        const ImVec2 file_pos={p.x+126*z+ImGui::CalcTextSize(document_name.c_str()).x+26*z,p.y-2*z};
        ImGui::SetCursorScreenPos(file_pos);
        if (ImGui::InvisibleButton("file.menu",{76*z,24*z})) ImGui::OpenPopup("file_commands");
        if (ImGui::IsItemHovered()) dl->AddRectFilled(file_pos,{file_pos.x+76*z,file_pos.y+24*z},packed(t.panel_alt),t.rounding*z);
        icon(dl,icon_texture("file-text"),{file_pos.x+4*z,file_pos.y+3*z},18*z,packed(t.text));
        dl->AddText({file_pos.x+29*z,file_pos.y+3*z},packed(t.text),"File");
        icon(dl,icon_texture("chevron-down"),{file_pos.x+57*z,file_pos.y+6*z},12*z,packed(t.muted));
        if (ImGui::BeginPopup("file_commands")) {
            for (const auto& command:ui::file_menu_commands()) popup_command(command,190);
            ImGui::EndPopup();
        }
        float action_x=file_pos.x+88*z;
        for (const auto& command:ui::edit_toolbar_commands()) {
            command_button(command,{action_x,p.y-2*z},{29*z,24*z},false);
            action_x+=32*z;
        }
        dl->AddLine({p.x,p.y+27*z},{p.x+width,p.y+27*z},packed(t.border));
        static int selected_tab=0;
        static bool previous_sketch_mode=false;
        if (state_.sketch_mode!=previous_sketch_mode) {
            selected_tab=state_.sketch_mode?1:0;
            previous_sketch_mode=state_.sketch_mode;
        }
        float tab_x=p.x+2*z;
        const auto tabs=ui::toolbar_catalog();
        for (int i=0;i<static_cast<int>(tabs.size());++i) {
            if (tabs[i].contextual && !state_.sketch_mode) continue;
            const std::string label(tabs[i].label);
            const float tab_width=ImGui::CalcTextSize(label.c_str()).x+23*z;
            ImGui::SetCursorScreenPos({tab_x,p.y+29*z});
            ImGui::PushID(i);
            if (ImGui::InvisibleButton("tab",{tab_width,23*z})) selected_tab=i;
            ImGui::PopID();
            dl->AddText({tab_x+9*z,p.y+33*z},packed(i==selected_tab?t.text:t.muted),label.c_str());
            if (i==selected_tab) dl->AddLine({tab_x+8*z,p.y+52*z},{tab_x+tab_width-8*z,p.y+52*z},packed(t.accent),2*z);
            tab_x+=tab_width;
        }
        dl->AddLine({p.x,p.y+55*z},{p.x+width,p.y+55*z},packed(t.border));
        const auto& groups=tabs[selected_tab].groups;
        const float group_width=std::min(t.ribbon_group_width*z,(width-4*z)/std::max<size_t>(1,groups.size()));
        for (size_t i=0;i<groups.size();++i) {
            const auto& group=groups[i];
            const float x=p.x+i*group_width;
            const size_t primary_count=std::min(group.primary_count,group.commands.size());
            const float item_width=(group_width-8*z)/static_cast<float>(primary_count);
            for (size_t j=0;j<primary_count;++j)
                command_button(group.commands[j],{x+2*z+j*item_width,p.y+60*z},{item_width,43*z},true);
            const std::string popup_id="group_"+std::to_string(i);
            ImGui::SetCursorScreenPos({x+4*z,p.y+118*z});
            ImGui::PushID(popup_id.c_str());
            if (ImGui::InvisibleButton("menu",{group_width-10*z,22*z})) ImGui::OpenPopup("commands");
            if (ImGui::IsItemHovered()) dl->AddRectFilled({x+4*z,p.y+118*z},{x+group_width-6*z,p.y+140*z},packed(t.panel_alt));
            centered_text(dl,{x+group_width*0.5f-6*z,p.y+122*z},group.label,packed(t.muted));
            icon(dl,icon_texture("chevron-down"),{x+group_width-25*z,p.y+122*z},13*z,packed(t.muted));
            if (ImGui::BeginPopup("commands")) {
                for (const auto& command:group.commands) popup_command(command,190);
                ImGui::EndPopup();
            }
            ImGui::PopID();
            dl->AddLine({x+group_width-1*z,p.y+61*z},{x+group_width-1*z,p.y+140*z},packed(t.border));
        }
        ImGui::SetCursorScreenPos({p.x,p.y+142*z});
        ImGui::Dummy({width,1*z});
    }, ImGuiWindowFlags_NoScrollbar);
    if (state_.browser) panel("Browser", ui::labels.browser, t, [&] {
        auto row=[&](const char* text,int depth,bool has_children,bool expanded,bool show_eye,bool visible) {
            ImDrawList* dl=ImGui::GetWindowDrawList();
            const ImVec2 q=ImGui::GetCursorScreenPos();
            const float indent=depth*15*z;
            if (has_children)
                icon(dl,icon_texture(expanded?"chevron-down":"chevron-right"),{q.x+indent,q.y+2*z},13*z,packed(t.muted));
            if (show_eye)
                icon(dl,icon_texture(visible?"eye":"eye-off"),{q.x+indent+17*z,q.y+1*z},15*z,packed(t.muted));
            dl->AddText({q.x+indent+39*z,q.y+1*z},packed(depth==0?t.text:t.muted),text);
            ImGui::Dummy({1,23*z});
        };
        const std::string browser_name=engine_?engine_->session().document.name:"Nuovo documento";
        row(browser_name.c_str(),0,true,true,true,true);
        row("Impostazioni documento",1,false,false,false,true);
        row("Viste salvate",1,false,false,true,true);
        row("Origine",1,true,true,true,true);
        row("Piano XY",2,false,false,true,true);
        row("Piano XZ",2,false,false,true,true);
        row("Piano YZ",2,false,false,true,true);
        const bool has_bodies=engine_ && !engine_->session().document.bodies.empty();
        row("Corpi",1,has_bodies,has_bodies,true,true);
        if (has_bodies) for (const auto& body:engine_->session().document.bodies) {
            const ImVec2 q=ImGui::GetCursorScreenPos();
            const ImVec2 eye_pos={q.x+47*z,q.y+1*z};
            ImGui::PushID(body.id.c_str());
            ImGui::SetCursorScreenPos({q.x+67*z,q.y});
            ImGui::InvisibleButton("body_row",{std::max(50.0f,ImGui::GetContentRegionAvail().x-67*z),23*z});
            if (ImGui::BeginPopupContextItem("body_context")) {
                if (ImGui::MenuItem("Modifica feature")) {
                    const auto& document=engine_->session().document;
                    for (auto it=document.timeline.rbegin();it!=document.timeline.rend();++it) {
                        const auto found=document.features.find(*it);
                        if (found!=document.features.end() && found->second.data.is_object() &&
                            found->second.data.contains("body_id") && found->second.data["body_id"].is_string() &&
                            found->second.data["body_id"].get<std::string>()==body.id) {
                            open_feature_editor(*it); break;
                        }
                    }
                }
                ImGui::EndPopup();
            }
            ImGui::SetCursorScreenPos(eye_pos);
            if (ImGui::InvisibleButton("visibility",{18*z,18*z})) {
                const auto result=engine_->execute("body.visibility",{{"body",body.id},{"visible",!body.visible}});
                if (!result.ok) state_.status_message=result.error;
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s",body.visible?"Nascondi corpo":"Mostra corpo");
            ImGui::PopID();
            ImDrawList* dl=ImGui::GetWindowDrawList();
            icon(dl,icon_texture(body.visible?"eye":"eye-off"),eye_pos,15*z,packed(t.muted));
            icon(dl,icon_texture("box"),{q.x+67*z,q.y+1*z},15*z,packed(t.muted));
            dl->AddText({q.x+87*z,q.y+1*z},packed(t.text),body.name.c_str());
            ImGui::SetCursorScreenPos(q); ImGui::Dummy({1,23*z});
        }
        row("Schizzi",1,false,false,true,true);
    });
    if (state_.properties) panel("Proprietà", ui::labels.properties, t, [&] {
        ImGui::TextDisabled("%s", ui::labels.selection); ImGui::Separator();
        ImGui::TextDisabled("Nessun elemento selezionato");
    });
    if (state_.timeline) panel("Timeline", nullptr, t, [&] {
        ImDrawList* dl=ImGui::GetWindowDrawList();
        const ImVec2 q=ImGui::GetCursorScreenPos();
        dl->AddText({q.x,q.y+2*z},packed(t.muted),ui::labels.timeline);
        const char* controls[]={"skip-back","step-back","play","step-forward","skip-forward"};
        for (int i=0;i<5;++i)
            icon(dl,icon_texture(controls[i]),{q.x+105*z+i*28*z,q.y},17*z,packed(t.muted));
        dl->AddLine({q.x+255*z,q.y+9*z},{q.x+ImGui::GetContentRegionAvail().x,q.y+9*z},packed(t.border));
        ImGui::Dummy({1,20*z});
        if (!engine_ || engine_->session().document.timeline.empty()) ImGui::TextDisabled("Il documento è vuoto");
        else {
            const auto& document=engine_->session().document;
            const auto& snapshot=regenerator_.evaluate(document);
            std::string pending_action,pending_id;
            bool after_edit_feature=false;
            for (const auto& id:document.timeline) {
                const auto found=document.features.find(id);
                if (found==document.features.end()) continue;
                const auto& feature=found->second;
                const auto error=std::find_if(snapshot.features.begin(),snapshot.features.end(),[&](const auto& row){return row.id==id;});
                const bool failed=error!=snapshot.features.end() && !error->error.empty();
                const char* glyph=feature.type=="primitive.box"?"box":feature.type=="primitive.cylinder"?"cylinder":"circle";
                const ImVec2 pos=ImGui::GetCursorScreenPos();
                ImGui::PushID(id.c_str());
                ImGui::InvisibleButton("feature",{100*z,23*z});
                const bool double_clicked=ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
                const bool hovered=ImGui::IsItemHovered();
                if (ImGui::BeginPopupContextItem("feature_context")) {
                    if (ImGui::MenuItem("Modifica feature")) open_feature_editor(id);
                    if (ImGui::MenuItem(feature.suppressed?"Ripristina":"Sopprimi")) {pending_action="feature.suppress";pending_id=id;}
                    if (ImGui::MenuItem("Elimina")) {pending_action="feature.delete";pending_id=id;}
                    ImGui::EndPopup();
                }
                if (hovered) ImGui::SetTooltip("%s%s",feature.name.c_str(),failed?error->error.c_str():"\nDoppio clic per modificare");
                ImGui::PopID();
                const bool at_marker=state_.primitive_editor_open && state_.primitive_feature_id==id;
                icon(dl,icon_texture(glyph),{pos.x+3*z,pos.y+2*z},17*z,packed(failed?t.error:t.muted));
                dl->AddText({pos.x+24*z,pos.y+2*z},packed(failed?t.error:after_edit_feature?t.muted:t.text),feature.name.c_str());
                if (at_marker) { dl->AddLine({pos.x+99*z,pos.y-2*z},{pos.x+99*z,pos.y+26*z},packed(t.accent),2*z); after_edit_feature=true; }
                if (double_clicked) open_feature_editor(id);
                ImGui::SameLine();
            }
            if (!pending_action.empty()) {
                nlohmann::json params={{"feature_id",pending_id}};
                if (pending_action=="feature.suppress") params["suppressed"]=!document.features.at(pending_id).suppressed;
                const auto result=engine_->execute(pending_action,params);
                if (!result.ok) state_.status_message=result.error;
                if (result.ok && pending_id==state_.primitive_feature_id) state_.primitive_editor_open=false;
            }
        }
        if (glyph_sample_) ImGui::TextUnformatted("àèéìòù ÀÈÉÌÒÙ");
    }, ImGuiWindowFlags_NoScrollbar);
    panel("Viewport", nullptr, t, [&] {
        const ImVec2 a = ImGui::GetCursorScreenPos();
        const ImVec2 avail = ImGui::GetContentRegionAvail();
        const ImVec2 b = {a.x+avail.x,a.y+avail.y};
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ui::Viewport viewport{a.x,a.y,avail.x,avail.y};
        ImGui::InvisibleButton("canvas",avail);
        if (ImGui::IsItemHovered()) {
            const auto& io=ImGui::GetIO();
            if (io.MouseWheel!=0) state_.camera.zoom(io.MouseWheel,io.MousePos.x,io.MousePos.y,viewport);
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
                if (io.KeyShift) state_.camera.pan(io.MouseDelta.x,io.MouseDelta.y,avail.y);
                else state_.camera.orbit(io.MouseDelta.x,io.MouseDelta.y);
            }
        }
        if (engine_) {
            const auto preview=ui::feature_preview(state_,engine_->session().document);
            const auto& doc=preview;
            const std::string revision=doc::to_json(doc).dump();
            if (revision!=surface_revision_) {
                surface_revision_=revision; surfaces_.clear();
                const auto& snap=regenerator_.evaluate(doc);
                state_.primitive_preview_error.clear();
                if (state_.primitive_editor_open) for (const auto& feature:snap.features)
                    if (feature.id==state_.primitive_feature_id) state_.primitive_preview_error=feature.error;
                for (const auto& body:doc.bodies) {
                    if (!body.visible) continue;
                    const auto found=std::find_if(snap.bodies.begin(),snap.bodies.end(),[&](const auto& b){return b.id==body.id;});
                    if (found==snap.bodies.end() || !found->final) continue;
                    auto built=mesh::build(*found->final,0.2);
                    if (built) surfaces_.push_back(std::move(*built.value));
                    else state_.status_message=built.error;
                }
                if (!surfaces_.empty() && !camera_fitted_) {
                    auto lo=surfaces_.front().minimum,hi=surfaces_.front().maximum;
                    for (const auto& surface:surfaces_) {
                        lo.x=std::min(lo.x,surface.minimum.x);lo.y=std::min(lo.y,surface.minimum.y);lo.z=std::min(lo.z,surface.minimum.z);
                        hi.x=std::max(hi.x,surface.maximum.x);hi.y=std::max(hi.y,surface.maximum.y);hi.z=std::max(hi.z,surface.maximum.z);
                    }
                    state_.camera.fit(lo,hi); camera_fitted_=true;
                }
                if (surfaces_.empty()) camera_fitted_=false;
            }
        }
        dl->AddRectFilled(a,b,packed(t.viewport));
        dl->PushClipRect(a,b,true);
        const float cx=(a.x+b.x)*0.5f;
        const float cy=(a.y+b.y)*0.5f;
        if (surfaces_.empty()) {
            const float step=t.grid_step*z;
            const float run=(b.y-a.y)/t.grid_slope;
            const int count=static_cast<int>(std::ceil((b.x-a.x+run)/step))+1;
            for (int i=-count;i<=count;++i) {
                const float x=cx+i*step;
                const ImU32 color=packed(i%5==0?t.grid_major:t.grid_minor);
                dl->AddLine({x,a.y},{x+run,b.y},color);
                dl->AddLine({x,a.y},{x-run,b.y},color);
            }
            dl->AddLine({a.x,cy-(a.x-cx)*t.grid_slope},{b.x,cy-(b.x-cx)*t.grid_slope},packed(t.axis_x),1.5f*z);
            dl->AddLine({a.x,cy+(a.x-cx)*t.grid_slope},{b.x,cy+(b.x-cx)*t.grid_slope},packed(t.axis_y),1.5f*z);
        } else {
            const double extent=std::max(20.0,state_.camera.distance*0.8);
            const double step=std::max(5.0,std::pow(10.0,std::floor(std::log10(extent/5.0))));
            const int count=std::min(100,int(extent/step));
            for (int i=-count;i<=count;++i) {
                auto line=[&](kernel::Vec3 p,kernel::Vec3 q,ui::Color color) {
                    const auto u=state_.camera.project(p,viewport),v=state_.camera.project(q,viewport);
                    if (u.visible && v.visible) dl->AddLine({float(u.x),float(u.y)},{float(v.x),float(v.y)},packed(color),i==0?1.5f:1.0f);
                };
                line({i*step,-extent,0},{i*step,extent,0},i==0?t.axis_y:(i%5==0?t.grid_major:t.grid_minor));
                line({-extent,i*step,0},{extent,i*step,0},i==0?t.axis_x:(i%5==0?t.grid_major:t.grid_minor));
            }
            for (const auto& surface:surfaces_) draw_surface(dl,surface,state_.camera,viewport,t,z);
        }
        if (surfaces_.empty()) dl->AddCircleFilled({cx,cy},2.5f*z,packed(t.accent));
        const float cube=t.viewcube_size*z;
        const ImVec2 cube_a={b.x-cube-t.padding*z,a.y+t.padding*z};
        draw_view_cube(dl,state_.camera,cube_a,cube,t,ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left),ImGui::GetIO().MousePos);
        const ImVec2 nav_a={cx-t.nav_width*z*0.5f,b.y-t.nav_height*z-t.padding*z};
        const ImVec2 nav_b={nav_a.x+t.nav_width*z,nav_a.y+t.nav_height*z};
        dl->AddRectFilled(nav_a,nav_b,packed(t.panel),t.rounding*z);
        dl->AddRect(nav_a,nav_b,packed(t.border),t.rounding*z);
        const char* nav_icons[]={"hand","orbit","zoom-in","maximize"};
        for (int i=0;i<4;++i)
            icon(dl,icon_texture(nav_icons[i]),{nav_a.x+26*z+i*50*z,nav_a.y+6*z},18*z,packed(t.muted));
        dl->PopClipRect();
    });
    draw_overlays(t);
}
bool Shell::open_feature_editor(std::string_view feature_id) {
    return engine_ && ui::begin_feature_edit(state_,engine_->session().document,feature_id);
}
void Shell::invoke_command(std::string_view id) {
    if (!engine_) return;
    const auto availability=ui::command_availability(id,engine_);
    if (!availability.enabled) { state_.status_message=availability.reason; return; }
    if (id=="param.list" || id=="param.set" || id=="param.delete") {
        state_.parameters_open=true;
        return;
    }
    if (id=="primitive.add") {
        state_.primitive_editor_open=false; state_.primitive_create_open=true; state_.primitive_kind=0; state_.primitive_error.clear();
        for (auto& buffer:state_.primitive_dimensions) std::snprintf(buffer.data(),buffer.size(),"40 mm");
        std::snprintf(state_.primitive_dimensions[2].data(),state_.primitive_dimensions[2].size(),"20 mm");
        return;
    }
    if (id=="document.open" || id=="document.saveAs" ||
        id=="export.stl" || (id=="document.save" && engine_->session().path.empty())) {
        state_.file_command= id=="document.save" ? "document.saveAs" : std::string(id);
        state_.file_dialog_open=true;
        state_.file_path.fill('\0');
        return;
    }
    const auto result=engine_->execute(id);
    state_.status_message=result.ok ? std::string() : result.error;
}
void Shell::draw_overlays(const ui::Theme& t) {
    const float z=static_cast<float>(scale_);
    const ImVec2 display=ImGui::GetIO().DisplaySize;
    if ((state_.primitive_create_open || state_.primitive_editor_open) && engine_) {
        const bool creating=state_.primitive_create_open;
        bool* open=creating?&state_.primitive_create_open:&state_.primitive_editor_open;
        const float panel_width=std::min(380*z,display.x*0.28f);
        ImGui::SetNextWindowPos({std::max(t.padding*z,display.x-panel_width-2*t.padding*z),
                                 t.toolbar_height*z+8*t.padding*z},ImGuiCond_Appearing);
        ImGui::SetNextWindowSize({panel_width,0},ImGuiCond_Appearing);
        if (ImGui::Begin(creating?"Crea primitiva###primitive_panel":"Modifica feature###primitive_panel",open,ImGuiWindowFlags_AlwaysAutoResize)) {
            const char* types[]={"Box","Cilindro","Sfera"};
            if (!creating) {
                const auto found=engine_->session().document.features.find(state_.primitive_feature_id);
                if (found==engine_->session().document.features.end()) *open=false;
                else state_.primitive_kind=found->second.type=="primitive.box"?0:found->second.type=="primitive.cylinder"?1:2;
            }
            ImGui::BeginDisabled(!creating);
            ImGui::Combo("Tipo",&state_.primitive_kind,types,3);
            ImGui::EndDisabled();
            const std::array<const char*,3> keys=state_.primitive_kind==0?std::array<const char*,3>{"width","depth","height"}:
                state_.primitive_kind==1?std::array<const char*,3>{"radius","height",nullptr}:std::array<const char*,3>{"radius",nullptr,nullptr};
            const std::array<const char*,3> labels=state_.primitive_kind==0?std::array<const char*,3>{"Larghezza","Profondità","Altezza"}:
                state_.primitive_kind==1?std::array<const char*,3>{"Raggio","Altezza",nullptr}:std::array<const char*,3>{"Raggio",nullptr,nullptr};
            for (int i=0;i<3;++i) if (keys[i]) {
                ImGui::TextUnformatted(labels[i]); ImGui::SetNextItemWidth(-1); ImGui::PushID(i);
                ImGui::InputText("##dimension",state_.primitive_dimensions[i].data(),state_.primitive_dimensions[i].size());
                if (ImGui::IsItemEdited()) state_.primitive_error.clear();
                ImGui::PopID();
            }
            const auto& feedback=state_.primitive_error.empty()?state_.primitive_preview_error:state_.primitive_error;
            if (!feedback.empty() && !creating) ImGui::TextColored(vec(t.error),"%s",feedback.c_str());
            else if (!state_.primitive_error.empty()) ImGui::TextColored(vec(t.error),"%s",state_.primitive_error.c_str());
            if (ImGui::Button(creating?"Crea":"OK")) {
                nlohmann::json dimensions=nlohmann::json::object();
                for (int i=0;i<3;++i) if (keys[i]) dimensions[keys[i]]=state_.primitive_dimensions[i].data();
                const auto result=creating?
                    engine_->execute("primitive.add",{{"kind",std::array<const char*,3>{"box","cylinder","sphere"}[state_.primitive_kind]},{"dimensions",dimensions}}):
                    engine_->execute("feature.edit",{{"feature_id",state_.primitive_feature_id},{"dimensions",dimensions}});
                if (result.ok) { *open=false; state_.primitive_error.clear(); state_.primitive_preview_error.clear(); }
                else state_.primitive_error=result.error;
            }
            ImGui::SameLine(); if (ImGui::Button("Annulla") || ImGui::IsKeyPressed(ImGuiKey_Escape)) *open=false;
        }
        ImGui::End();
    }
    if (state_.palette_open) {
        ImGui::SetNextWindowPos({(display.x-t.palette_width*z)*0.5f,t.toolbar_height*z+24*z},ImGuiCond_Always);
        ImGui::SetNextWindowSize({t.palette_width*z,t.palette_height*z},ImGuiCond_Always);
        if (ImGui::Begin("Comandi##palette",&state_.palette_open,ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove)) {
            ImGui::TextColored(vec(t.muted),"COMANDI");
            ImGui::Separator();
            if (state_.palette_focus) { ImGui::SetKeyboardFocusHere(); state_.palette_focus=false; }
            ImGui::InputTextWithHint("##search","Cerca un comando...",state_.palette_query.data(),state_.palette_query.size());
            if (ImGui::IsKeyPressed(ImGuiKey_Escape)) state_.palette_open=false;
            ImGui::BeginChild("results",{0,0},false);
            if (engine_) for (const auto& spec:engine_->registry().specs()) {
                auto lower=[](std::string value) { for (char& c:value) c=static_cast<char>(std::tolower(static_cast<unsigned char>(c))); return value; };
                const std::string query=lower(state_.palette_query.data());
                if (!query.empty() && lower(spec.label).find(query)==std::string::npos && lower(spec.id).find(query)==std::string::npos) continue;
                const auto availability=ui::command_availability(spec.id,engine_);
                ImGui::PushID(spec.id.c_str());
                ImGui::BeginDisabled(!availability.enabled);
                const bool clicked=ImGui::Selectable((spec.label+"  ##command").c_str());
                ImGui::EndDisabled();
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("%s",availability.enabled?spec.id.c_str():availability.reason.c_str());
                if (clicked) { invoke_command(spec.id); state_.palette_open=false; }
                ImGui::PopID();
            }
            ImGui::EndChild();
        }
        ImGui::End();
    }
    if (state_.parameters_open) {
        ImGui::SetNextWindowPos({(display.x-t.parameter_width*z)*0.5f,
            std::max(t.toolbar_height*z+24*z,(display.y-t.parameter_height*z)*0.5f)},ImGuiCond_Appearing);
        ImGui::SetNextWindowSize({std::min(t.parameter_width*z,display.x-24*z),std::min(t.parameter_height*z,display.y-24*z)},ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Cambia parametri##parameters",&state_.parameters_open)) {
            ImGui::TextColored(vec(t.muted),"%s",ui::labels.parameters);
            ImGui::Separator();
            if (ImGui::Button("+##add_parameter")) {
                state_.parameter_editing=true;
                state_.editing_parameter_name.clear();
                state_.parameter_name.fill('\0');
                state_.parameter_unit.fill('\0');
                state_.parameter_expression.fill('\0');
                state_.parameter_comment.fill('\0');
                state_.parameter_error.clear();
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Aggiungi parametro");
            ImGui::SameLine(); ImGui::TextDisabled("Invio conferma · Esc annulla");
            if (state_.parameter_editing && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                state_.parameter_editing=false;
                state_.editing_parameter_name.clear();
                state_.parameter_error.clear();
            }
            bool commit_requested=false;
            if (ImGui::BeginTable("parameters",5,ImGuiTableFlags_Borders|ImGuiTableFlags_RowBg|
                                  ImGuiTableFlags_SizingStretchProp|ImGuiTableFlags_ScrollY,{0,270*z})) {
                const auto& h=ui::parameter_table_labels;
                ImGui::TableSetupColumn(h.name,ImGuiTableColumnFlags_WidthStretch,1.3f);
                ImGui::TableSetupColumn(h.unit,ImGuiTableColumnFlags_WidthStretch,0.6f);
                ImGui::TableSetupColumn(h.expression,ImGuiTableColumnFlags_WidthStretch,2.0f);
                ImGui::TableSetupColumn(h.value,ImGuiTableColumnFlags_WidthStretch,1.0f);
                ImGui::TableSetupColumn(h.comment,ImGuiTableColumnFlags_WidthStretch,1.5f);
                ImGui::TableSetupScrollFreeze(0,1);
                ImGui::PushStyleColor(ImGuiCol_Text,vec(t.table_header_text));
                ImGui::TableHeadersRow();
                ImGui::PopStyleColor();
                auto edit_row=[&](const char* identity) {
                    ImGui::PushID(identity);
                    ImGui::TableNextRow();
                    const auto preview=engine_?ui::preview_parameter(state_,*engine_):doc::EvaluatedParameter{};
                    const std::string error=!state_.parameter_error.empty()?state_.parameter_error:preview.error;
                    auto input=[&](int column,const char* id,auto& buffer,bool show_error) {
                        ImGui::TableSetColumnIndex(column);
                        if (show_error && !error.empty()) {
                            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg,packed(t.error_cell));
                            ImGui::PushStyleColor(ImGuiCol_Text,vec(t.error));
                        }
                        ImGui::SetNextItemWidth(-1.0f);
                        if (ImGui::InputText(id,buffer.data(),buffer.size(),ImGuiInputTextFlags_EnterReturnsTrue)) commit_requested=true;
                        if (ImGui::IsItemEdited()) state_.parameter_error.clear();
                        if (show_error && !error.empty()) {
                            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s",error.c_str());
                            ImGui::TextWrapped("%s",error.c_str());
                            ImGui::PopStyleColor();
                        }
                    };
                    input(0,"##name",state_.parameter_name,false);
                    input(1,"##unit",state_.parameter_unit,false);
                    input(2,"##expression",state_.parameter_expression,true);
                    ImGui::TableSetColumnIndex(3);
                    if (!error.empty()) {
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg,packed(t.error_cell));
                        ImGui::TextColored(vec(t.error),"Errore");
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s",error.c_str());
                    }
                    else if (preview.value) ImGui::Text("%.6g",*preview.value);
                    input(4,"##comment",state_.parameter_comment,false);
                    ImGui::PopID();
                };
                if (engine_) for (const auto& row:engine::evaluated_parameters_json(engine_->session().document)) {
                    const std::string name=row.value("name",std::string());
                    if (state_.parameter_editing && state_.editing_parameter_name==name) {
                        edit_row(name.c_str()); continue;
                    }
                    ImGui::PushID(name.c_str());
                    ImGui::TableNextRow();
                    auto cell=[&](int column,const std::string& value,bool is_error) {
                        ImGui::TableSetColumnIndex(column);
                        if (is_error) ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg,packed(t.error_cell));
                        if (is_error) ImGui::PushStyleColor(ImGuiCol_Text,vec(t.error));
                        if (ImGui::Selectable((value+"##cell"+std::to_string(column)).c_str())) {
                            state_.parameter_editing=true;
                            state_.editing_parameter_name=name;
                            std::snprintf(state_.parameter_name.data(),state_.parameter_name.size(),"%s",name.c_str());
                            std::snprintf(state_.parameter_unit.data(),state_.parameter_unit.size(),"%s",row.value("unit",std::string()).c_str());
                            std::snprintf(state_.parameter_expression.data(),state_.parameter_expression.size(),"%s",row.value("expression",std::string()).c_str());
                            std::snprintf(state_.parameter_comment.data(),state_.parameter_comment.size(),"%s",row.value("comment",std::string()).c_str());
                            state_.parameter_error.clear();
                        }
                        if (is_error) {
                            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s",row.value("error",std::string()).c_str());
                            ImGui::PopStyleColor();
                        }
                    };
                    const bool bad=!row.value("error",std::string()).empty();
                    cell(0,name,false);
                    cell(1,row.value("unit",std::string()),false);
                    cell(2,row.value("expression",std::string()),bad);
                    char formatted_value[64]{};
                    if (row["value"].is_number()) std::snprintf(formatted_value,sizeof(formatted_value),"%.6g",row["value"].get<double>());
                    else std::snprintf(formatted_value,sizeof(formatted_value),"%s","Errore");
                    cell(3,formatted_value,bad);
                    cell(4,row.value("comment",std::string()),false);
                    ImGui::PopID();
                }
                if (state_.parameter_editing && state_.editing_parameter_name.empty()) edit_row("new");
                ImGui::EndTable();
            }
            if (state_.parameter_editing) {
                if (ImGui::Button("Conferma") || commit_requested) {
                    if (engine_) ui::submit_parameter(state_,*engine_);
                }
                ImGui::SameLine();
                if (ImGui::Button("Annulla")) { state_.parameter_editing=false; state_.parameter_error.clear(); }
                if (!state_.editing_parameter_name.empty()) {
                    ImGui::SameLine();
                    if (ImGui::Button("Elimina") && engine_) {
                        const auto result=engine_->execute("param.delete",{{"name",state_.editing_parameter_name}});
                        state_.parameter_error=result.ok?std::string():result.error;
                        if (result.ok) state_.parameter_editing=false;
                    }
                }
            }
        }
        ImGui::End();
    }
    if (state_.file_dialog_open) {
        ImGui::SetNextWindowPos({(display.x-t.file_dialog_width*z)*0.5f,(display.y-160*z)*0.5f},ImGuiCond_Appearing);
        ImGui::SetNextWindowSize({t.file_dialog_width*z,160*z},ImGuiCond_Appearing);
        const char* dialog_title=state_.file_command=="export.stl"?"Esporta STL##file":"Percorso documento##file";
        if (ImGui::Begin(dialog_title,&state_.file_dialog_open,ImGuiWindowFlags_NoResize)) {
            ImGui::TextUnformatted(state_.file_command=="export.stl"?"Nome file .stl (salvato in export/)":"File .sagomacad");
            ImGui::InputText("##path",state_.file_path.data(),state_.file_path.size());
            if (ImGui::Button("Conferma") && engine_) {
                const auto result=engine_->execute(state_.file_command,{{"path",state_.file_path.data()}});
                state_.status_message=result.ok?std::string():result.error;
                if (result.ok) state_.file_dialog_open=false;
            }
            if (!state_.status_message.empty()) ImGui::TextColored(vec(t.axis_x),"%s",state_.status_message.c_str());
        }
        ImGui::End();
    }
}
bool Shell::glyphs_available() const {
    if (!imgui_ready_) return false;
    ImFont* font=ImGui::GetIO().Fonts->Fonts[0];
    for (char32_t c : U"àèéìòù ÀÈÉÌÒÙ ĀāČčŁłŽž") {
        if (c != U' ' && c != U'\0' && !font->IsGlyphInFont(static_cast<ImWchar>(c))) return false;
    }
    return true;
}
void Shell::render() {
    ImGui_ImplOpenGL3_NewFrame(); ImGui_ImplSDL3_NewFrame(); ImGui::NewFrame();
    style(ui::theme(state_.theme == "light"), scale_);
    draw();
    ImGui::Render();
    int w=0,h=0; SDL_GetWindowSizeInPixels(window_,&w,&h);
    const auto& bg=ui::theme(state_.theme=="light").background;
    glViewport(0,0,w,h); glClearColor(bg.r,bg.g,bg.b,bg.a); glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window_);
}
bool Shell::screenshot(const std::string& path, std::string& error) {
    if (path.empty()) { error="empty screenshot path"; return false; }
    int w=0,h=0; SDL_GetWindowSizeInPixels(window_,&w,&h);
    if (w<=0 || h<=0 || w>8192 || h>8192) { error="invalid framebuffer size"; return false; }
    std::vector<unsigned char> pixels(static_cast<size_t>(w)*h*4);
    glReadBuffer(GL_FRONT);
    glPixelStorei(GL_PACK_ALIGNMENT,1);
    glReadPixels(0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
    std::vector<unsigned char> flipped(pixels.size());
    const size_t stride=static_cast<size_t>(w)*4;
    for (int y=0;y<h;++y) std::copy_n(pixels.data()+static_cast<size_t>(h-y-1)*stride,stride,flipped.data()+static_cast<size_t>(y)*stride);
    std::error_code ec;
    const auto parent=std::filesystem::path(path).parent_path();
    if (!parent.empty()) std::filesystem::create_directories(parent,ec);
    if (ec || !stbi_write_png(path.c_str(),w,h,4,flipped.data(),w*4)) { error="cannot write PNG"; return false; }
    return true;
}
nlohmann::json Shell::inspect() const {
    return {{"theme",state_.theme},{"width",state_.width},{"height",state_.height},
        {"panels",{{"browser",state_.browser},{"properties",state_.properties},{"timeline",state_.timeline}}},
        {"viewport",{{"grid",true},{"axes",true},{"title_visible",false}}},
        {"docking",true},{"fixed_layout",true},{"tabs_visible",false},
        {"palette_open",state_.palette_open},{"parameters_open",state_.parameters_open},
        {"feature_edit_open",state_.primitive_editor_open},{"feature_edit_id",state_.primitive_feature_id},
        {"parameters",engine_?engine::evaluated_parameters_json(engine_->session().document):nlohmann::json::array()}};
}
}
