#pragma once
#include "ui/state.hpp"
#include "ui/layout.hpp"
#include "ui/theme.hpp"
#include "engine/command.hpp"
#include "regen/regenerator.hpp"
#include "mesh/surface.hpp"
#include "solver/sketch_solver.hpp"
#include <SDL3/SDL_video.h>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

namespace sagomacad {
class Shell {
public:
    Shell(int width, int height, int scale, bool hidden, bool light);
    ~Shell();
    Shell(const Shell&) = delete;
    Shell& operator=(const Shell&) = delete;
    bool valid() const { return imgui_ready_; }
    const std::string& error() const { return error_; }
    bool poll();
    void render();
    bool screenshot(const std::string& path, std::string& error, bool viewport_only=false,
                    int output_width=0,int output_height=0);
    void set_glyph_sample(bool enabled) { glyph_sample_ = enabled; }
    bool glyphs_available() const;
    nlohmann::json inspect() const;
    void set_settings_file(const std::string& path);
    ui::State& state() { return state_; }
    void bind_engine(engine::Engine* engine) { engine_ = engine; }
    bool open_feature_editor(std::string_view feature_id);
private:
    SDL_Window* window_ = nullptr;
    SDL_GLContext context_ = nullptr;
    std::string error_;
    ui::State state_;
    engine::Engine* engine_ = nullptr;
    regen::Regenerator regenerator_;
    std::string surface_revision_;
    std::vector<mesh::Surface> surfaces_;
    bool camera_fitted_ = false;
    std::string sketch_revision_;
    std::map<std::string,solver::SketchResult> sketch_solutions_;
    int scale_ = 1;
    bool imgui_ready_ = false;
    ui::LayoutEngine layout_;
    std::string settings_file_,settings_cache_;
    void save_settings();
    bool glyph_sample_ = false;
    ui::Viewport current_viewport_{};
    std::unordered_map<std::string, unsigned int> icon_textures_;
    unsigned int icon_texture(std::string_view name);
    void draw();
    void draw_overlays(const ui::Theme& theme);
    void invoke_command(std::string_view id);
    void fit_camera();
    void sync_sketch();
    void draw_sketch_canvas(const ui::Theme&,bool hovered);
    void draw_sketch_panels(const ui::Theme&);
    void draw_timeline(const ui::Theme&);
};
}
