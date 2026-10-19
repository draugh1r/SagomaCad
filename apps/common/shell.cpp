#include "shell.hpp"
#include "ui/surface_editor.hpp"
#include "view_cube_widget.hpp"
#include "ui/theme.hpp"
#include "ui/toolbar_catalog.hpp"
#include "ui/parameter_editor.hpp"
#include "ui/feature_editor.hpp"
#include "ui/model_editor.hpp"
#include "ui/widgets.hpp"
#include "ui/frame.hpp"
#include "ui/sketch_view.hpp"
#include <fstream>
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
    c[ImGuiCol_FrameBgActive] = vec(t.visual.accent_soft); c[ImGuiCol_TitleBg] = vec(t.panel);
    c[ImGuiCol_TitleBgActive] = vec(t.panel); c[ImGuiCol_MenuBarBg] = vec(t.panel);
    c[ImGuiCol_Button] = vec(t.panel_alt); c[ImGuiCol_ButtonHovered] = vec(t.border);
    c[ImGuiCol_ButtonActive] = vec(t.visual.accent_soft); c[ImGuiCol_Header] = vec(t.panel_alt);
    c[ImGuiCol_HeaderHovered] = vec(t.border); c[ImGuiCol_HeaderActive] = vec(t.visual.accent_soft);
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
void panel(const char* name, const char* label, const ui::Theme& t, ui::LayoutRect r, auto contents, ImGuiWindowFlags extra = 0, bool* collapsed=nullptr, float z=1, bool collapse_right=false) {
    if (!r.visible()) return;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize,{0,0});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,0);
    constexpr auto fixed = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBringToFrontOnFocus;
    if (ui::frame::panel(name,r,fixed | extra)) {
        if (collapsed) {
            const auto cursor=ImGui::GetCursorScreenPos();
            ImGui::SetCursorScreenPos({r.x+r.width-t.layout.collapse_button*z,r.y+t.padding*z});
            const bool arrow_right=*collapsed!=collapse_right;
            if (ImGui::SmallButton(arrow_right?">##collapse":"<##collapse")) *collapsed=!*collapsed;
            ImGui::SetCursorScreenPos(cursor);
        }
        if ((!collapsed || !*collapsed) && r.width>2*t.padding*z && r.height>2*t.padding*z) {
            if (label) { ui::frame::heading(label,t,z,false,collapsed?t.layout.collapse_button*z:0); }
            contents();
        }
    }
    ImGui::End();
    ImGui::PopStyleVar(3);
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
struct DrawTriangle { ImVec2 a,b,c; double da,db,dc,depth; ui::Color color; };
bool hides(const DrawTriangle& tri,ImVec2 point,double depth) {
    const double denominator=(tri.b.y-tri.c.y)*(tri.a.x-tri.c.x)+(tri.c.x-tri.b.x)*(tri.a.y-tri.c.y);
    if (std::abs(denominator)<1e-10) return false;
    const double wa=((tri.b.y-tri.c.y)*(point.x-tri.c.x)+(tri.c.x-tri.b.x)*(point.y-tri.c.y))/denominator;
    const double wb=((tri.c.y-tri.a.y)*(point.x-tri.c.x)+(tri.a.x-tri.c.x)*(point.y-tri.c.y))/denominator;
    const double wc=1-wa-wb;
    if (wa<-1e-6 || wb<-1e-6 || wc<-1e-6) return false;
    const double surface_depth=1/(wa/tri.da+wb/tri.db+wc/tri.dc);
    return surface_depth<depth-std::max(1e-5,depth*1e-5);
}
void draw_surfaces(ImDrawList* dl,const std::vector<mesh::Surface>& surfaces,const ui::Camera& camera,ui::Viewport viewport,const ui::Theme& theme,float scale) {
    std::vector<const mesh::Surface*> ordered;
    for (const auto& surface:surfaces) ordered.push_back(&surface);
    std::sort(ordered.begin(),ordered.end(),[](const auto* a,const auto* b) {
        return std::array{a->minimum.x,a->minimum.y,a->minimum.z,a->maximum.x,a->maximum.y,a->maximum.z}<
               std::array{b->minimum.x,b->minimum.y,b->minimum.z,b->maximum.x,b->maximum.y,b->maximum.z};
    });
    std::vector<DrawTriangle> triangles;
    const auto eye=camera.eye();
    for (const auto* surface:ordered) for (const auto& tri:surface->geometry.triangles) {
        const auto a=camera.project(tri.vertices[0],viewport),b=camera.project(tri.vertices[1],viewport),c=camera.project(tri.vertices[2],viewport);
        if (!a.visible || !b.visible || !c.visible) continue;
        const auto& n=tri.normal;
        if(theme.solid.a>=1.f&&n.x*(eye.x-tri.vertices[0].x)+n.y*(eye.y-tri.vertices[0].y)+n.z*(eye.z-tri.vertices[0].z)<=0)continue;
        const double light=std::max(0.0,n.x*0.42+n.y*(-0.35)+n.z*0.84);
        triangles.push_back({{float(a.x),float(a.y)},{float(b.x),float(b.y)},{float(c.x),float(c.y)},a.depth,b.depth,c.depth,(a.depth+b.depth+c.depth)/3.0,shaded(theme.solid,light)});
    }
    std::sort(triangles.begin(),triangles.end(),[](const auto& a,const auto& b){return a.depth>b.depth;});
    const auto flags=dl->Flags;
    dl->Flags&=~ImDrawListFlags_AntiAliasedFill;
    for (const auto& tri:triangles) dl->AddTriangleFilled(tri.a,tri.b,tri.c,packed(tri.color));
    dl->Flags=flags;
    for (const auto* surface:ordered) for (const auto& edge:surface->geometry.edges) {
      std::vector<ImVec2> visible;
      auto flush=[&] {
          if (visible.size()>1) dl->AddPolyline(visible.data(),static_cast<int>(visible.size()),packed(theme.solid_edge),0,1.15f*scale);
          visible.clear();
      };
      for (std::size_t i=1;i<edge.points.size();++i) {
        const auto p=camera.project(edge.points[i-1],viewport),q=camera.project(edge.points[i],viewport);
        if (!p.visible || !q.visible) { flush(); continue; }
        const int segments=std::clamp(static_cast<int>(std::ceil(std::hypot(q.x-p.x,q.y-p.y)/4.0)),1,256);
        for (int s=0;s<segments;++s) {
            const double t0=double(s)/segments,t1=double(s+1)/segments,tm=(t0+t1)*0.5;
            const ImVec2 midpoint={float(p.x+(q.x-p.x)*tm),float(p.y+(q.y-p.y)*tm)};
            const double depth=1/((1-tm)/p.depth+tm/q.depth);
            bool occluded=false;
            for (const auto& tri:triangles) if (hides(tri,midpoint,depth)) { occluded=true; break; }
            if (occluded) flush();
            else {
                if (visible.empty()) visible.push_back({float(p.x+(q.x-p.x)*t0),float(p.y+(q.y-p.y)*t0)});
                visible.push_back({float(p.x+(q.x-p.x)*t1),float(p.y+(q.y-p.y)*t1)});
            }
        }
      }
      flush();
    }
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
    gl_viewport_.reset();
    design_icons_.reset();
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
    sync_sketch();
    const auto& t = ui::theme(state_.theme == "light");
    const float z = static_cast<float>(scale_);
    state_.view_transition.advance(state_.camera,ImGui::GetIO().DeltaTime);
    const ImVec2 screen = ImGui::GetIO().DisplaySize;
    const auto& input=ImGui::GetIO();
    if ((input.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_K)) ||
        (!input.KeyCtrl && !input.WantTextInput && ImGui::IsKeyPressed(ImGuiKey_S))) {
        state_.palette_open=true;
        state_.palette_focus=true;
    }
    if (input.KeyCtrl && !input.WantTextInput && ImGui::IsKeyPressed(ImGuiKey_Z)) invoke_command("edit.undo");
    if (input.KeyCtrl && !input.WantTextInput && ImGui::IsKeyPressed(ImGuiKey_Y)) invoke_command("edit.redo");
    if(!input.WantTextInput&&!input.KeyCtrl&&!state_.palette_open&&!state_.sketch_mode&&ImGui::IsKeyPressed(ImGuiKey_F))invoke_command("feature.fillet");
    if(!input.WantTextInput&&!input.KeyCtrl&&!state_.palette_open&&!state_.sketch_mode&&ImGui::IsKeyPressed(ImGuiKey_H))invoke_command("feature.hole");
    if(!input.WantTextInput&&!input.KeyCtrl&&!state_.palette_open&&ImGui::IsKeyPressed(ImGuiKey_E))invoke_command("feature.extrude");
    if (input.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_N)) invoke_command("document.new");
    if (input.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_O)) invoke_command("document.open");
    if (input.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S))
        invoke_command(input.KeyShift?"document.saveAs":"document.save");
    if (state_.sketch_mode!=state_.previous_sketch_mode) {
        state_.selected_tab=state_.sketch_mode?1:0;
        state_.previous_sketch_mode=state_.sketch_mode;
    }
    const int layout_tab=state_.selected_tab;
    const auto& layout=layout_.compute(screen.x,screen.y,z,state_,t,glyph_sample_);
    layout_.render(ImGui::GetBackgroundDrawList());
    auto texture=[&](const ui::ToolbarCommand& c,float size,bool disabled) { return c.design_icon.empty()?icon_texture(c.icon):design_icon_texture(c.design_icon,c.accent,size,disabled); };
    auto invoke=[&](std::string_view id) { invoke_command(id); };
    auto command_button=[&](const ui::ToolbarCommand& c,ImVec2 pos,ImVec2 size,bool label) {
        ui::ribbon_button(c,{pos.x,pos.y,size.x,size.y},label,t,z,engine_,texture,invoke,ui::command_selected(c.id,state_));
    };
    auto popup_command=[&](const ui::ToolbarCommand& c,float width) {
        ui::popup_command(c,width,t,z,engine_,texture,invoke,ui::command_selected(c.id,state_));
    };
    panel("SagomaCad  |  Solido", nullptr, t, layout.topbar, [&] {
        ImDrawList* dl=ImGui::GetWindowDrawList();
        const ImVec2 p=ImGui::GetCursorScreenPos();
        const float width=ImGui::GetContentRegionAvail().x;
        dl->AddText(p,packed(t.visual.sel_edge),"SagomaCad");
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
            if(ImGui::MenuItem("Preferenze"))state_.preferences_open=true;
            ImGui::EndPopup();
        }
        float action_x=file_pos.x+88*z;
        for (const auto& command:ui::edit_toolbar_commands()) {
            command_button(command,{action_x,p.y-2*z},{29*z,24*z},false);
            action_x+=32*z;
        }
        dl->AddLine({p.x,p.y+27*z},{p.x+width,p.y+27*z},packed(t.border));
        auto& selected_tab=state_.selected_tab;
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
    }, ImGuiWindowFlags_NoScrollbar);
    if (state_.selected_tab!=layout_tab) layout_.compute(screen.x,screen.y,z,state_,t,glyph_sample_);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{t.padding*z,0});
    panel("Ribbon",nullptr,t,layout.ribbon,[&] {
        auto* dl=ImGui::GetWindowDrawList();
        const auto tabs=ui::toolbar_catalog();
        const auto& groups=tabs[layout.tab_index].groups;
        for (size_t i=0;i<layout.group_count;++i) {
            const auto& g=layout.groups[i];
            for (size_t j=0;j<g.button_count;++j) {
                const auto& r=g.buttons[j];
                ui::ribbon_button(groups[i].commands[j],r,true,t,z,engine_,texture,invoke,ui::command_selected(groups[i].commands[j].id,state_));
            }
            const auto& r=g.menu;
            const std::string popup_id="group_"+std::to_string(i);
            ImGui::SetCursorScreenPos({r.x,r.y});
            ImGui::PushID(popup_id.c_str());
            if (r.visible() && ImGui::InvisibleButton("menu",{r.width,r.height})) ImGui::OpenPopup("commands");
            if (ImGui::IsItemHovered()) dl->AddRectFilled({r.x,r.y},{r.x+r.width,r.y+r.height},packed(t.panel_alt));
            icon(dl,icon_texture("chevron-down"),{r.x+r.width-t.layout.menu_icon_right*z,r.y+t.layout.label_top*z},t.layout.menu_icon_size*z,packed(t.muted));
            if (ImGui::BeginPopup("commands")) {
                for (const auto& command:groups[i].commands) popup_command(command,190);
                ImGui::EndPopup();
            }
            ImGui::PopID();
        }
        layout_.render(dl);
    },ImGuiWindowFlags_NoScrollbar);
    ImGui::PopStyleVar();
    if (state_.browser) panel("Browser", ui::labels.browser, t, layout.browser, [&] {
        auto row=[&](const char* text,int depth,bool has_children,bool expanded,bool show_eye,bool visible,const char* glyph="folder") {
            ImDrawList* dl=ImGui::GetWindowDrawList();
            const ImVec2 q=ImGui::GetCursorScreenPos();
            const float indent=depth*15*z;
            if (has_children)
                icon(dl,icon_texture(expanded?"chevron-down":"chevron-right"),{q.x+indent,q.y+2*z},13*z,packed(t.muted));
            if (show_eye)
                icon(dl,icon_texture(visible?"eye":"eye-off"),{q.x+indent+17*z,q.y+1*z},15*z,packed(t.muted));
            dl->AddImage(ImTextureID(design_icon_texture(glyph,std::string_view(glyph)=="sketch"?ui::IconAccent::Sketch:ui::IconAccent::Solid,t.board.browser_icon)),{q.x+indent+39*z,q.y+z},{q.x+indent+(39+t.board.browser_icon)*z,q.y+(1+t.board.browser_icon)*z});
            dl->AddText({q.x+indent+(39+t.board.browser_icon+t.spacing)*z,q.y+1*z},packed(depth==0?t.text:t.muted),text);
            ImGui::Dummy({1,23*z});
        };
        const std::string browser_name=engine_?engine_->session().document.name:"Nuovo documento";
        row(browser_name.c_str(),0,true,true,true,true,"new");
        row("Impostazioni documento",1,false,false,false,true,"params");
        row("Viste salvate",1,false,false,true,true,"camera");
        row("Origine",1,true,true,true,true,"origin");
        row("Piano XY",2,false,false,true,true,"plane");
        row("Piano XZ",2,false,false,true,true,"plane");
        row("Piano YZ",2,false,false,true,true,"plane");
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
            dl->AddImage(ImTextureID(design_icon_texture("body",ui::IconAccent::Solid,t.board.browser_icon)),{q.x+67*z,q.y+z},{q.x+(67+t.board.browser_icon)*z,q.y+(1+t.board.browser_icon)*z});
            dl->AddText({q.x+87*z,q.y+1*z},packed(t.text),body.name.c_str());
            ImGui::SetCursorScreenPos(q); ImGui::Dummy({1,23*z});
        }
        bool has_sketches=false;
        if(engine_)for(const auto& [id,f]:engine_->session().document.features)if(f.type=="sketch")has_sketches=true;
        row("Schizzi",1,has_sketches,has_sketches,true,true);
        state_.sketch_browser.clear();
        if(engine_){
            auto context=engine::model_context(engine_->session().document,state_.model.open?state_.model.feature_id:"");
            if(state_.sketch_mode){auto at=std::find(context.timeline.begin(),context.timeline.end(),state_.sketch_view_id);if(at!=context.timeline.end())context.rollback=std::min(context.rollback,std::size_t(at-context.timeline.begin())+1);}
            const auto snapshot=regenerator_.evaluate(context);
            for(const auto& id:context.timeline){auto found=context.features.find(id);if(found==context.features.end())continue;const auto& f=found->second;if(f.type!="sketch")continue;
                const auto q=ImGui::GetCursorScreenPos();const bool shown=regen::sketch_visible(context,snapshot,id,state_.sketch_mode?state_.sketch_view_id:state_.model.open?state_.model.sketch:"");
                auto& item=state_.sketch_browser[id];item.visible=shown;item.eye={q.x+47*z,q.y,18*z,18*z};
                ImGui::PushID(id.c_str());ImGui::SetCursorScreenPos({item.eye[0],item.eye[1]});
                if(ImGui::InvisibleButton("sketch_eye",{18*z,18*z})){auto r=engine_->execute("sketch.visibility",{{"sketch",id},{"visible",!shown}});if(!r.ok)state_.status_message=r.error;}
                if(ImGui::IsItemHovered())ImGui::SetTooltip("%s",shown?"Nascondi schizzo":"Mostra schizzo per riutilizzare i profili. Lo schizzo usato viene nascosto automaticamente.");
                icon(ImGui::GetWindowDrawList(),icon_texture(shown?"eye":"eye-off"),{item.eye[0],item.eye[1]+z},15*z,packed(t.muted));
                auto* sketch_dl=ImGui::GetWindowDrawList();
                sketch_dl->AddImage(ImTextureID(design_icon_texture("sketch",ui::IconAccent::Sketch,t.board.browser_icon,!shown)),{q.x+67*z,q.y+z},{q.x+(67+t.board.browser_icon)*z,q.y+(1+t.board.browser_icon)*z});
                ImGui::SetCursorScreenPos({q.x+(67+t.board.browser_icon+t.spacing)*z,q.y});
                if(ImGui::Selectable(f.name.c_str(),state_.sketch_view_id==id||state_.selected_profile_sketch==id)) {state_.selected_profile_sketch=id;state_.selected_profiles.clear();if(state_.model.open){state_.model.sketch=id;state_.model.profiles.clear();}}
                auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();item.row={a.x,a.y,b.x-a.x,b.y-a.y};
                if(ImGui::IsItemHovered()&&ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))open_feature_editor(id);
                if(ImGui::IsItemHovered())ImGui::SetTooltip("%s",shown?"Seleziona e premi E per Estrudi. Doppio clic per modificare lo schizzo.":"Schizzo nascosto, non eliminato. Usa l'occhio per mostrarlo o scegli Estrudi.");
                if(ImGui::BeginPopupContextItem()) {
                    if(ImGui::MenuItem("Estrudi")){state_.selected_profile_sketch=id;state_.selected_profiles.clear();ui::begin_model(state_,*engine_,false);}
                    if(ImGui::MenuItem("Modifica schizzo"))open_feature_editor(id);
                    if(ImGui::MenuItem(shown?"Nascondi schizzo":"Mostra schizzo"))engine_->execute("sketch.visibility",{{"sketch",id},{"visible",!shown}});
                    if(ImGui::MenuItem("Visibilità automatica"))engine_->execute("sketch.visibility",{{"sketch",id},{"visible",nullptr}});
                    ImGui::EndPopup();
                }
                ImGui::PopID();ImGui::SetCursorScreenPos(q);ImGui::Dummy({1,23*z});
            }
        }
    },0,&state_.browser_collapsed,z);
    if (state_.properties) panel("Proprietà", ui::labels.properties, t, layout.properties, [&] {
        ImGui::TextDisabled("%s", ui::labels.selection); ImGui::Separator();
        ImGui::TextDisabled("Nessun elemento selezionato");
    },0,&state_.properties_collapsed,z,true);
    if (state_.timeline) panel("Timeline", nullptr, t, layout.timeline, [&] {
        draw_timeline(t);
        if (glyph_sample_) ImGui::TextUnformatted("àèéìòù ÀÈÉÌÒÙ");
    }, ImGuiWindowFlags_NoScrollbar);
    panel("Viewport", nullptr, t, layout.viewport, [&] {
        const ImVec2 a = ImGui::GetCursorScreenPos();
        const ImVec2 avail = ImGui::GetContentRegionAvail();
        const ImVec2 b = {a.x+avail.x,a.y+avail.y};
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ui::Viewport viewport{a.x,a.y,avail.x,avail.y};
        current_viewport_=viewport;
        const ImVec2 nav_a={(a.x+b.x)*0.5f-t.nav_width*z*0.5f,b.y-t.nav_height*z-t.padding*z};
        const ImVec2 nav_b={nav_a.x+t.nav_width*z,nav_a.y+t.nav_height*z};
        ImGui::SetNextItemAllowOverlap();
        ImGui::InvisibleButton("canvas",avail);
        const auto mouse=ImGui::GetIO().MousePos;
        const bool over_navigation=mouse.x>=nav_a.x && mouse.x<=nav_b.x && mouse.y>=nav_a.y && mouse.y<=nav_b.y;
        const ui::CubePoint cube_origin{b.x-t.cube.widget_size*z-t.padding*z,a.y+t.padding*z};
        const bool over_cube=mouse.x>=cube_origin.x && mouse.x<=cube_origin.x+t.cube.widget_size*z &&
                             mouse.y>=cube_origin.y && mouse.y<=cube_origin.y+t.cube.widget_size*z;
        const bool canvas_hovered=ImGui::IsItemHovered(state_.sketch_tool.started||state_.model.open?ImGuiHoveredFlags_AllowWhenBlockedByActiveItem:ImGuiHoveredFlags_None);
        if (canvas_hovered && !over_navigation && !over_cube && !state_.cube_interaction.pressed) {
            const auto& io=ImGui::GetIO();
            if (io.MouseWheel!=0 || ImGui::IsMouseDragging(ImGuiMouseButton_Middle) ||
                (state_.navigation_mode>=0 && ImGui::IsMouseDragging(ImGuiMouseButton_Left))) state_.view_transition.cancel();
            if (io.MouseWheel!=0) state_.camera.zoom(io.MouseWheel,io.MousePos.x,io.MousePos.y,viewport);
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
                if (io.KeyShift) state_.camera.orbit(io.MouseDelta.x,io.MouseDelta.y);
                else state_.camera.pan(io.MouseDelta.x,io.MouseDelta.y,avail.y);
            }
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                if (state_.navigation_mode==0) state_.camera.pan(io.MouseDelta.x,io.MouseDelta.y,avail.y);
                else if (state_.navigation_mode==1) state_.camera.orbit(io.MouseDelta.x,io.MouseDelta.y);
                else if (state_.navigation_mode==2) state_.camera.zoom(-io.MouseDelta.y*0.05,viewport.x+viewport.width*0.5,viewport.y+viewport.height*0.5,viewport);
            }
        }
        if (engine_) {
            auto preview=ui::feature_preview(state_,engine_->session().document);
            if(state_.model.open){
                preview=engine::model_context(engine_->session().document,state_.model.feature_id);
                ui::update_model_operation(state_,*engine_);
                const auto params=ui::model_params(state_);const auto revision=doc::to_json(engine_->session().document).dump()+params.dump()+state_.model.feature_id;
                if(revision!=model_revision_){model_revision_=revision;model_preview_=engine::model_preview(engine_->session().document,state_.model.feature_id,state_.model.revolve?"feature.revolve":"feature.extrude",params);model_surfaces_.clear();if(model_preview_.surface)model_surfaces_.push_back(*model_preview_.surface);}
                state_.model.preview_error=model_preview_.error;
            }else if(state_.surface.open){preview=engine::model_context(engine_->session().document,state_.surface.feature_id);
            }else if(state_.sketch_mode){
                auto at=std::find(preview.timeline.begin(),preview.timeline.end(),state_.sketch_view_id);if(at!=preview.timeline.end())preview.rollback=std::size_t(at-preview.timeline.begin())+1;
            }
            const auto& doc=preview;
            const std::string revision=doc::to_json(doc).dump();
            if (revision!=surface_revision_) {
                surface_revision_=revision; surfaces_.clear();surface_bodies_.clear();
                const auto& snap=regenerator_.evaluate(doc);
                state_.primitive_preview_error.clear();
                if (state_.primitive_editor_open) for (const auto& feature:snap.features)
                    if (feature.id==state_.primitive_feature_id) state_.primitive_preview_error=feature.error;
                for (const auto& body:doc.bodies) {
                    if (!body.visible) continue;
                    const auto found=std::find_if(snap.bodies.begin(),snap.bodies.end(),[&](const auto& b){return b.id==body.id;});
                    if (found==snap.bodies.end() || !found->final) continue;
                    auto built=mesh::build(*found->final,0.2);
                    if (built) {surfaces_.push_back(std::move(*built.value));surface_bodies_.push_back(body.id);}
                    else state_.status_message=built.error;
                }
                if (!surfaces_.empty() && !camera_fitted_) {
                    fit_camera();
                }
                if (surfaces_.empty()) camera_fitted_=false;
            }
        }
        dl->AddRectFilled(a,b,packed(t.viewport));
        dl->PushClipRect(a,b,true);
        const float cx=(a.x+b.x)*0.5f;
        if(!gl_viewport_)gl_viewport_=std::make_unique<ui::GlViewport>();
        const bool gpu_available=gl_viewport_->initialize();
        if (!gpu_available&&!state_.sketch_mode) {
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
        }
        if(!gl_viewport_)gl_viewport_=std::make_unique<ui::GlViewport>();
        auto gpu_theme=t;
        ui::GridPlane grid;grid.enabled=!state_.sketch_mode||state_.sketch_grid;
        if(state_.sketch_mode&&sketch_solutions_.contains(state_.sketch_view_id)){
            const auto& sketch=sketch_solutions_.at(state_.sketch_view_id).sketch;
            grid.origin=ui::sketch_world(sketch,{0,0});const auto u=ui::sketch_world(sketch,{1,0}),v=ui::sketch_world(sketch,{0,1});
            grid.u={u.x-grid.origin.x,u.y-grid.origin.y,u.z-grid.origin.z};grid.v={v.x-grid.origin.x,v.y-grid.origin.y,v.z-grid.origin.z};
        }
        gl_viewport_->set_grid(grid);
        const bool gpu=gl_viewport_->enqueue(dl,surfaces_,surface_revision_,state_.camera,viewport,gpu_theme,z);
        if(!gpu){state_.status_message="Viewport CPU: "+gl_viewport_->error();draw_surfaces(dl,surfaces_,state_.camera,viewport,t,z);}
        if(state_.surface.open&&surface_preview_.surface){auto display=surfaces_;for(size_t i=0;i<display.size()&&i<surface_bodies_.size();++i)if(surface_bodies_[i]==surface_preview_.source_body)display[i]=*surface_preview_.surface;draw_surfaces(dl,display,state_.camera,viewport,t,z);}
        if(state_.model.open&&!model_surfaces_.empty()){
            auto preview_theme=t;preview_theme.solid=state_.model.operation==1?t.model.cut:t.model.preview;preview_theme.solid_edge=state_.model.operation==1?t.model.cut:t.accent;preview_theme.solid_edge.a=.85f;
            draw_surfaces(dl,model_surfaces_,state_.camera,viewport,preview_theme,z);
        }
        draw_geometry_canvas(t,canvas_hovered&&!over_navigation&&!over_cube);
        draw_sketch_canvas(t,canvas_hovered && !over_navigation && !over_cube);
        draw_model_canvas(t,canvas_hovered&&!over_navigation&&!over_cube);
        if (surfaces_.empty()) {
            const auto origin=state_.camera.project({0,0,0},viewport);
            if (origin.visible) dl->AddCircleFilled({float(origin.x),float(origin.y)},2.5f*z,packed(t.accent));
        }
        const auto cube_action=draw_view_cube_widget(dl,state_.camera,state_.cube_interaction,state_.view_transition,cube_origin,t,z,canvas_hovered);
        if (cube_action.kind!=ui::CubeKind::None) {
            const auto before=state_.camera;
            ui::cube_apply(state_.camera,cube_action);
            if (cube_action.kind==ui::CubeKind::Home) {
                if (surfaces_.empty()) { state_.camera.target={}; state_.camera.distance=ui::Camera{}.distance; }
                else fit_camera();
            }
            const auto destination=state_.camera;
            state_.camera=before;
            state_.view_transition.begin(before,destination,t.cube.transition_duration);
        }
        dl->AddRectFilled(nav_a,nav_b,packed(t.panel),t.rounding*z);
        dl->AddRect(nav_a,nav_b,packed(t.border),t.rounding*z);
        const char* nav_icons[]={"hand","orbit","zoom-in","maximize"};
        const char* nav_labels[]={"Pan · trascina con il tasto sinistro","Orbita · trascina con il tasto sinistro","Zoom · trascina con il tasto sinistro","Adatta alla finestra"};
        for (int i=0;i<4;++i) {
            ImGui::SetCursorScreenPos({nav_a.x+15*z+i*50*z,nav_a.y+2*z});
            ImGui::PushID(i);
            const bool clicked=ImGui::InvisibleButton("navigation",{40*z,t.nav_height*z-4*z});
            const bool hovered=ImGui::IsItemHovered();
            if (clicked) {
                if (i==3) { state_.view_transition.cancel(); fit_camera(); }
                else state_.navigation_mode=state_.navigation_mode==i?-1:i;
            }
            if (hovered) ImGui::SetTooltip("%s",nav_labels[i]);
            icon(dl,icon_texture(nav_icons[i]),{nav_a.x+26*z+i*50*z,nav_a.y+6*z},18*z,packed(state_.navigation_mode==i?t.accent:t.muted));
            ImGui::PopID();
        }
        dl->PopClipRect();
    });
    if (!state_.browser_collapsed)
        ui::splitter("browser_separator",layout.browser_separator,true,state_.browser_width,t.sidebar_width-t.layout.panel_border,t.layout.browser_min,t.layout.browser_max,z,t);
    if (!state_.properties_collapsed)
        ui::splitter("properties_separator",layout.properties_separator,true,state_.properties_width,t.inspector_width-t.layout.panel_border,t.layout.properties_min,t.layout.properties_max,z,t);
    ui::splitter("timeline_separator",layout.timeline_separator,false,state_.timeline_height,t.timeline_height,t.layout.timeline_min,t.layout.timeline_max,z,t);
    draw_overlays(t);
    draw_sketch_panels(t);
    if(state_.preferences_open&&engine_){
        ui::frame::window("Preferenze",&state_.preferences_open,t,z,{t.file_dialog_width*z,0},false,ImGuiWindowFlags_AlwaysAutoResize);
        bool hide=engine_->session().hide_sketch_after_feature;
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,t.layout.panel_border*z);
        if(ImGui::Checkbox("Nascondi lo schizzo dopo la creazione di una feature",&hide))engine_->execute("preferences.set",{{"hide_sketch_after_feature",hide}});
        ImGui::PopStyleVar();
        ImGui::End();
    }
}
void Shell::fit_camera() {
    if (surfaces_.empty()) return;
    auto lo=surfaces_.front().minimum,hi=surfaces_.front().maximum;
    for (const auto& surface:surfaces_) {
        lo.x=std::min(lo.x,surface.minimum.x); lo.y=std::min(lo.y,surface.minimum.y); lo.z=std::min(lo.z,surface.minimum.z);
        hi.x=std::max(hi.x,surface.maximum.x); hi.y=std::max(hi.y,surface.maximum.y); hi.z=std::max(hi.z,surface.maximum.z);
    }
    state_.camera.fit(lo,hi); camera_fitted_=true;
}
bool Shell::open_feature_editor(std::string_view feature_id) {
    if(engine_) {
        auto it=engine_->session().document.features.find(std::string(feature_id));
        if(it!=engine_->session().document.features.end()&&(it->second.type=="feature.fillet"||it->second.type=="feature.chamfer"||it->second.type=="feature.hole")){ui::begin_surface(state_,*engine_,it->second.type,std::string(feature_id));return true;}
        if(it!=engine_->session().document.features.end()&&(it->second.type=="feature.extrude"||it->second.type=="feature.revolve")){
            ui::begin_model(state_,*engine_,it->second.type=="feature.revolve",std::string(feature_id));return true;
        }
        if(it!=engine_->session().document.features.end() && it->second.type=="sketch") {
            auto r=engine_->execute("sketch.edit",{{"sketch",feature_id}});
            if(!r.ok)state_.status_message=r.error;
            return r.ok;
        }
    }
    return engine_ && ui::begin_feature_edit(state_,engine_->session().document,feature_id);
}
void Shell::invoke_command(std::string_view id) {
    if(id=="feature.fillet"||id=="feature.chamfer"||id=="feature.hole"){if(engine_)ui::begin_surface(state_,*engine_,std::string(id));return;}
    if(id=="selection.set"){state_.selection_filters_open=true;return;}
    if(id=="sketch.create"&&engine_){auto selected=engine_->execute("selection.get");if(selected.ok)for(const auto& ref:selected.value["refs"])if(ref.value("kind",std::string())=="face"){auto r=engine_->execute("sketch.create",{{"face",ref}});if(!r.ok)state_.status_message=r.error;else sync_sketch();return;}}

    if (!engine_) return;
    const auto availability=ui::command_availability(id,engine_);
    if (!availability.enabled) { state_.status_message=availability.reason; return; }
    if(id=="feature.extrude"||id=="feature.revolve"){ui::begin_model(state_,*engine_,id=="feature.revolve");return;}
    if (id=="sketch.create") {
        state_.sketch_plane_picker=true;state_.sketch_tool.begin("select");
        auto destination=state_.camera;destination.home();destination.target={};
        state_.view_transition.begin(state_.camera,destination,ui::theme(state_.theme=="light").cube.transition_duration);
        return;
    }
    if(id=="sketch.line"||id=="sketch.rect"||id=="sketch.circle"||id=="sketch.arc") {state_.sketch_tool.begin(std::string(id.substr(7)));return;}
    if(id=="sketch.dimension.add"||id=="sketch.dimension.set") {state_.sketch_tool.begin("dimension");return;}
    if(id=="sketch.constraint.add") {state_.sketch_constraint_open=true;return;}
    if(id=="sketch.point.move"||id=="sketch.inspect") {state_.sketch_tool.begin("select");return;}
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
    if(result.ok&&id=="sketch.finish"){
        auto destination=state_.camera;destination.home();
        state_.view_transition.begin(state_.camera,destination,ui::theme(state_.theme=="light").cube.transition_duration);
    }
}
void Shell::draw_overlays(const ui::Theme& t) {
    const float z=static_cast<float>(scale_);
    draw_model_panel(t);
    draw_surface_panel(t);
    const ImVec2 display=ImGui::GetIO().DisplaySize;
    if (!state_.status_message.empty() && !state_.file_dialog_open) {
        const auto& viewport=layout_.last().viewport;
        bool open=true;
        if (ui::frame::notice("Errore##status",&open,viewport,t,z)) {
            ImGui::PushStyleColor(ImGuiCol_Text,vec(t.error));
            ImGui::TextWrapped("%s",state_.status_message.c_str());
            ImGui::PopStyleColor();
        }
        ImGui::End();
        if (!open) state_.status_message.clear();
    }
    if ((state_.primitive_create_open || state_.primitive_editor_open) && engine_) {
        const bool creating=state_.primitive_create_open;
        bool* open=creating?&state_.primitive_create_open:&state_.primitive_editor_open;
        const auto r=layout_.last().contextual;
        if (ui::frame::dialog(creating?"Crea primitiva###primitive_panel":"Modifica feature###primitive_panel",open,r,t,z)) {
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
        if (ui::frame::window("Comandi##palette",&state_.palette_open,t,z,{t.palette_width*z,t.palette_height*z},false,ImGuiWindowFlags_NoResize)) {
            ImGui::TextColored(vec(t.muted),"COMANDI");
            ImGui::Separator();
            if (state_.palette_focus) { ImGui::SetKeyboardFocusHere(); state_.palette_focus=false; }
            ImGui::InputTextWithHint("##search","Cerca un comando...",state_.palette_query.data(),state_.palette_query.size());
            const auto search_a=ImGui::GetItemRectMin(),search_b=ImGui::GetItemRectMax();
            state_.palette_search={search_a.x,search_a.y,search_b.x-search_a.x,search_b.y-search_a.y};
            if (ImGui::IsKeyPressed(ImGuiKey_Escape)) state_.palette_open=false;
            ImGui::BeginChild("results",{0,0},false);
            if (engine_) for (const auto& spec:engine_->registry().specs()) {
                auto lower=[](std::string value) { for (char& c:value) c=static_cast<char>(std::tolower(static_cast<unsigned char>(c))); return value; };
                const std::string query=lower(state_.palette_query.data());
                if (!query.empty() && lower(spec.label).find(query)==std::string::npos && lower(spec.id).find(query)==std::string::npos) continue;
                const auto availability=ui::command_availability(spec.id,engine_);
                ImGui::PushID(spec.id.c_str());
                ImGui::BeginDisabled(!availability.enabled);
                const auto at=ImGui::GetCursorScreenPos();
                const bool clicked=ImGui::Selectable("##command",false,0,{0,std::max(ImGui::GetTextLineHeight(),t.layout.menu_icon*z)});
                ImGui::EndDisabled();
                const auto* artwork=ui::toolbar_command(spec.id);auto* dl=ImGui::GetWindowDrawList();
                if(artwork&&artwork->design_icon.empty())icon(dl,icon_texture(artwork->icon),at,t.layout.menu_icon*z,packed(availability.enabled?t.text:t.muted));
                else dl->AddImage(ImTextureID(design_icon_texture(artwork?artwork->design_icon:"placeholder",artwork?artwork->accent:ui::IconAccent::Solid,t.layout.menu_icon,!availability.enabled)),at,{at.x+t.layout.menu_icon*z,at.y+t.layout.menu_icon*z});
                dl->AddText({at.x+(t.layout.menu_icon+t.spacing)*z,at.y},packed(availability.enabled?t.text:t.muted),spec.label.c_str());
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("%s",availability.enabled?spec.id.c_str():availability.reason.c_str());
                if (clicked) { invoke_command(spec.id); state_.palette_open=false; }
                ImGui::PopID();
            }
            ImGui::EndChild();
        }
        ImGui::End();
    }
    if (state_.parameters_open) {
        if (ui::frame::window("Cambia parametri##parameters",&state_.parameters_open,t,z,{t.parameter_width*z,t.parameter_height*z},true)) {
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
        const char* dialog_title=state_.file_command=="export.stl"?"Esporta STL##file":"Percorso documento##file";
        if (ui::frame::window(dialog_title,&state_.file_dialog_open,t,z,{t.file_dialog_width*z,160*z},false,ImGuiWindowFlags_NoResize)) {
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
    ui::frame::reset_records();
    if(state_.design_board)draw_design_board(ui::theme(state_.theme=="light"));else draw();
    save_settings();
    ImGui::Render();
    int w=0,h=0; SDL_GetWindowSizeInPixels(window_,&w,&h);
    const auto& bg=ui::theme(state_.theme=="light").background;
    glViewport(0,0,w,h); glClearColor(bg.r,bg.g,bg.b,bg.a); glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window_);
}
bool Shell::screenshot(const std::string& path, std::string& error, bool viewport_only,
                       int output_width,int output_height) {
    if (path.empty()) { error="empty screenshot path"; return false; }
    int w=0,h=0; SDL_GetWindowSizeInPixels(window_,&w,&h);
    if (w<=0 || h<=0 || w>8192 || h>8192) { error="invalid framebuffer size"; return false; }
    const int framebuffer_width=w,framebuffer_height=h;
    int x=0,y=0;
    if (viewport_only) {
        const auto& display=ImGui::GetIO().DisplaySize;
        if (display.x<=0 || display.y<=0) { error="invalid viewport"; return false; }
        const double sx=static_cast<double>(w)/display.x,sy=static_cast<double>(h)/display.y;
        x=std::clamp(static_cast<int>(std::lround(current_viewport_.x*sx)),0,w);
        y=std::clamp(static_cast<int>(std::lround(current_viewport_.y*sy)),0,h);
        w=std::clamp(static_cast<int>(std::lround(current_viewport_.width*sx)),0,w-x);
        h=std::clamp(static_cast<int>(std::lround(current_viewport_.height*sy)),0,h-y);
        if (w<=0 || h<=0) { error="empty viewport"; return false; }
    }
    std::vector<unsigned char> pixels(static_cast<size_t>(w)*h*4);
    // A hidden/occluded native window has no reliable readable front buffer on Windows.
    // Redraw the completed ImGui frame into the back buffer before reading it.
    glViewport(0,0,framebuffer_width,framebuffer_height);
    const auto& background=ui::theme(state_.theme=="light").background;
    glClearColor(background.r,background.g,background.b,background.a);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glReadBuffer(GL_BACK);
    glPixelStorei(GL_PACK_ALIGNMENT,1);
    glReadPixels(x,viewport_only?framebuffer_height-y-h:0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
    std::vector<unsigned char> flipped(pixels.size());
    const size_t stride=static_cast<size_t>(w)*4;
    for (int y=0;y<h;++y) std::copy_n(pixels.data()+static_cast<size_t>(h-y-1)*stride,stride,flipped.data()+static_cast<size_t>(y)*stride);
    if (output_width>0 && output_height>0 && (output_width!=w || output_height!=h)) {
        if (output_width>2048 || output_height>2048 || output_width*output_height>2000000) {
            error="invalid output size"; return false;
        }
        std::vector<unsigned char> resized(static_cast<size_t>(output_width)*output_height*4);
        for (int row=0;row<output_height;++row) {
            const int source_y=std::min(h-1,static_cast<int>((static_cast<long long>(row)*h)/output_height));
            for (int col=0;col<output_width;++col) {
                const int source_x=std::min(w-1,static_cast<int>((static_cast<long long>(col)*w)/output_width));
                std::copy_n(flipped.data()+(static_cast<size_t>(source_y)*w+source_x)*4,4,
                            resized.data()+(static_cast<size_t>(row)*output_width+col)*4);
            }
        }
        flipped=std::move(resized);
        w=output_width; h=output_height;
    }
    std::error_code ec;
    const auto parent=std::filesystem::path(path).parent_path();
    if (!parent.empty()) std::filesystem::create_directories(parent,ec);
    if (ec || !stbi_write_png(path.c_str(),w,h,4,flipped.data(),w*4)) { error="cannot write PNG"; return false; }
    return true;
}
void Shell::set_settings_file(const std::string& path) {
    settings_file_=path;
    bool loaded=false;
    std::error_code ec;
    const auto length=std::filesystem::file_size(path,ec);
    if (!ec && length<=16384) {
        std::ifstream input(path,std::ios::binary);
        const auto json=nlohmann::json::parse(input,nullptr,false);
        ui::restore_layout_state(state_,json);
        loaded=json.is_object();
    }
    if(engine_)engine_->session().hide_sketch_after_feature=state_.hide_sketch_after_feature;
    settings_cache_=loaded?ui::layout_state_json(state_).dump():std::string{};
}
void Shell::save_settings() {
    if(engine_)state_.hide_sketch_after_feature=engine_->session().hide_sketch_after_feature;
    if (settings_file_.empty()) return;
    const auto value=ui::layout_state_json(state_).dump();
    if (value==settings_cache_) return;
    std::error_code ec;
    const auto path=std::filesystem::path(settings_file_);
    if (!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path(),ec);
    const auto temporary=settings_file_+".tmp";
    std::ofstream output(temporary,std::ios::binary|std::ios::trunc);
    if (ec || !output) { std::fprintf(stderr,"Cannot save UI layout settings\n"); return; }
    output<<value<<'\n'; output.close();
    if (!output || !SDL_RenamePath(temporary.c_str(),settings_file_.c_str())) {
        std::fprintf(stderr,"Cannot replace UI layout settings\n"); return;
    }
    settings_cache_=value;
}
nlohmann::json Shell::inspect() const {
    const auto& l=layout_.last();
    nlohmann::json rectangles=nlohmann::json::array();
    for (const auto& [name,r]:std::initializer_list<std::pair<const char*,ui::LayoutRect>>{
        {"topbar",l.topbar},{"ribbon",l.ribbon},{"browser",l.browser},{"viewport",l.viewport},
        {"properties",l.properties},{"timeline",l.timeline},{"contextual",l.contextual},
        {"header_separator",l.header_separator},{"browser_separator",l.browser_separator},{"properties_separator",l.properties_separator},{"timeline_separator",l.timeline_separator}})
        rectangles.push_back({{"id",name},{"x",r.x},{"y",r.y},{"width",r.width},{"height",r.height}});
    auto items=nlohmann::json::array();for(const auto& r:state_.timeline_items)items.push_back({{"id",r.id},{"x",r.x},{"y",r.y},{"width",r.width},{"height",r.height}});
    auto frames=nlohmann::json::array();for(const auto& r:ui::frame::records())frames.push_back({{"name",r.name},{"kind",r.kind},{"x",r.rect.x},{"y",r.rect.y},{"width",r.rect.width},{"height",r.rect.height},{"minimum",r.minimum},{"maximum",r.maximum}});
    return {{"theme",state_.theme},{"width",state_.width},{"height",state_.height},
        {"frames",frames},{"palette_search",state_.palette_search},{"design_board",state_.design_board},
        {"timeline_items",items},{"timeline_marker",{{"x",state_.timeline_marker_x},{"y",state_.timeline_marker_y},{"index",engine_?engine_->session().document.rollback:0}}},
        {"panels",{{"browser",state_.browser},{"properties",state_.properties},{"timeline",state_.timeline}}},
        {"viewport",{{"grid",true},{"axes",true},{"title_visible",false}}},
        {"docking",false},{"layout_engine","clay"},{"layout_scale",scale_},{"shell_rects",rectangles},{"layout_state",ui::layout_state_json(state_)},{"layout_error",layout_.error()},{"fixed_layout",true},{"tabs_visible",false},
        {"palette_open",state_.palette_open},{"parameters_open",state_.parameters_open},
        {"surface",{{"open",state_.surface.open},{"command",state_.surface.command},{"params",ui::surface_params(state_)},{"error",state_.surface.preview_error},{"fields",state_.surface.fields},{"handle",{{"x",state_.surface.handle_x},{"y",state_.surface.handle_y}}}}},{"topology_labels",state_.topology_labels},{"selection",engine_?engine_->session().selection:nlohmann::json::array()},{"feature_edit_open",state_.primitive_editor_open||(!state_.model.feature_id.empty()&&state_.model.open)},{"feature_edit_id",state_.model.open?state_.model.feature_id:state_.primitive_feature_id},{"model",ui::model_state_json(state_)},
        {"sketch",{{"mode",state_.sketch_mode},{"plane_picker",state_.sketch_plane_picker},{"id",state_.sketch_view_id},{"tool",state_.sketch_tool.kind},{"active_field",state_.sketch_tool.active_field},{"live_fields",state_.sketch_live_fields},{"dimension_rects",state_.sketch_dimension_rects},{"dimension_edit_rect",state_.sketch_dimension_edit_rect},{"error",state_.sketch_error}}},
        {"parameters",engine_?engine::evaluated_parameters_json(engine_->session().document):nlohmann::json::array()}};
}
}
