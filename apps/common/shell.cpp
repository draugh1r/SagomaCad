#include "shell.hpp"
#include "ui/theme.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <vector>

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
}
void panel(const char* name, const char* label, const ui::Theme& t, auto contents, ImGuiWindowFlags extra = 0) {
    constexpr auto fixed = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
    if (ImGui::Begin(name, nullptr, fixed | extra)) {
        if (label) { ImGui::TextColored(vec(t.muted), "%s", label); ImGui::Separator(); }
        contents();
    }
    ImGui::End();
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
    if (ImGui::GetCurrentContext()) ImGui::DestroyContext();
    if (context_) SDL_GL_DestroyContext(context_);
    if (window_) SDL_DestroyWindow(window_);
    SDL_Quit();
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
    const ImGuiID dock_id=ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
    static bool dock_initialized=false;
    if (!dock_initialized) {
        ImGui::DockBuilderRemoveNode(dock_id);
        ImGui::DockBuilderAddNode(dock_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dock_id,screen);
        ImGuiID middle=dock_id, header=0, timeline=0, browser=0, properties=0;
        ImGui::DockBuilderSplitNode(middle,ImGuiDir_Up,t.toolbar_height*z/screen.y,&header,&middle);
        ImGui::DockBuilderSplitNode(middle,ImGuiDir_Down,t.timeline_height*z/(screen.y-t.toolbar_height*z),&timeline,&middle);
        ImGui::DockBuilderSplitNode(middle,ImGuiDir_Left,t.sidebar_width*z/screen.x,&browser,&middle);
        ImGui::DockBuilderSplitNode(middle,ImGuiDir_Right,t.inspector_width*z/(screen.x-t.sidebar_width*z),&properties,&middle);
        for (ImGuiID node : {header,timeline,browser,properties,middle})
            ImGui::DockBuilderGetNode(node)->LocalFlags |= ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_NoResize;
        ImGui::DockBuilderDockWindow("SagomaCad  |  Solido",header);
        ImGui::DockBuilderDockWindow("Browser",browser);
        ImGui::DockBuilderDockWindow("Proprietà",properties);
        ImGui::DockBuilderDockWindow("Timeline",timeline);
        ImGui::DockBuilderDockWindow("Viewport",middle);
        ImGui::DockBuilderFinish(dock_id);
        dock_initialized=true;
    }
    panel("SagomaCad  |  Solido", nullptr, t, [&] {
        ImGui::TextColored(vec(t.accent), "SagomaCad"); ImGui::SameLine();
        ImGui::TextDisabled("  |  Nuovo documento");
        ImGui::SameLine(); ImGui::TextDisabled("   SOLIDO     SCHIZZO");
    }, ImGuiWindowFlags_NoScrollbar);
    if (state_.browser) panel("Browser", "BROWSER", t, [&] {
        ImGui::TextDisabled("DOCUMENTO"); ImGui::Separator();
        ImGui::TextUnformatted("  Origine"); ImGui::TextDisabled("    Piano XY");
        ImGui::TextDisabled("    Piano XZ"); ImGui::TextDisabled("    Piano YZ");
        ImGui::Spacing(); ImGui::TextDisabled("CORPI"); ImGui::Separator();
        ImGui::TextDisabled("  Nessun corpo");
    });
    if (state_.properties) panel("Proprietà", "PROPRIETÀ", t, [&] {
        ImGui::TextDisabled("SELEZIONE"); ImGui::Separator();
        ImGui::TextDisabled("Nessun elemento selezionato");
    });
    if (state_.timeline) panel("Timeline", "TIMELINE", t, [&] {
        ImGui::TextDisabled("CRONOLOGIA DELLE FEATURE"); ImGui::Separator();
        ImGui::TextDisabled("Il documento è vuoto");
        if (glyph_sample_) ImGui::TextUnformatted("àèéìòù ÀÈÉÌÒÙ");
    });
    panel("Viewport", nullptr, t, [&] {
        const ImVec2 a = ImGui::GetCursorScreenPos();
        const ImVec2 avail = ImGui::GetContentRegionAvail();
        const ImVec2 b = {a.x+avail.x,a.y+avail.y};
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(a,b,packed(t.viewport));
        dl->PushClipRect(a,b,true);
        const float step = 32.0f*z;
        const float cx = std::round((a.x+b.x)*0.5f/step)*step;
        const float cy = std::round((a.y+b.y)*0.5f/step)*step;
        for (float x=cx; x>=a.x; x-=step) dl->AddLine({x,a.y},{x,b.y},packed((std::fmod(std::round((x-cx)/step),5.0f)==0) ? t.grid_major:t.grid_minor));
        for (float x=cx+step; x<=b.x; x+=step) dl->AddLine({x,a.y},{x,b.y},packed((std::fmod(std::round((x-cx)/step),5.0f)==0) ? t.grid_major:t.grid_minor));
        for (float y=cy; y>=a.y; y-=step) dl->AddLine({a.x,y},{b.x,y},packed((std::fmod(std::round((y-cy)/step),5.0f)==0) ? t.grid_major:t.grid_minor));
        for (float y=cy+step; y<=b.y; y+=step) dl->AddLine({a.x,y},{b.x,y},packed((std::fmod(std::round((y-cy)/step),5.0f)==0) ? t.grid_major:t.grid_minor));
        dl->AddLine({a.x,cy},{b.x,cy},packed(t.axis_x),2*z);
        dl->AddLine({cx,a.y},{cx,b.y},packed(t.axis_y),2*z);
        dl->AddCircleFilled({cx,cy},3*z,packed(t.accent));
        dl->PopClipRect();
        ImGui::InvisibleButton("canvas",avail);
    });
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
        {"docking",true},{"fixed_layout",true},{"tabs_visible",false}};
}
}
