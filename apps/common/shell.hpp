#pragma once
#include "ui/state.hpp"
#include "ui/layout.hpp"
#include "ui/theme.hpp"
#include "ui/icons.hpp"
#include "ui/render/gl_viewport.hpp"
#include "engine/command.hpp"
#include "regen/regenerator.hpp"
#include "mesh/surface.hpp"
#include "solver/sketch_solver.hpp"
#include "engine/tools/model_preview.hpp"
#include "engine/tools/surface_preview.hpp"
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
    void bind_engine(engine::Engine* engine) { engine_ = engine; if(engine_){engine_->session().native_view=true;if(settings_file_.empty())state_.hide_sketch_after_feature=engine_->session().hide_sketch_after_feature;else engine_->session().hide_sketch_after_feature=state_.hide_sketch_after_feature;} }
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
    std::vector<std::string> surface_bodies_;
    engine::IdBuffer pick_buffer_;
    std::string pick_revision_,topology_revision_,surface_tool_revision_;
    nlohmann::json topology_entities_=nlohmann::json::array();
    engine::SurfacePreview surface_preview_;
    bool pick_pending_=false,pick_other_=false;
    float pick_x_=0,pick_y_=0;
    double pick_time_=0;
    std::vector<engine::PickHit> pick_candidates_;
    bool camera_fitted_ = false;
    std::string sketch_revision_;
    std::map<std::string,solver::SketchResult> sketch_solutions_;
    std::map<std::string,std::vector<engine::ModelProfile>> model_profiles_;
    std::string model_revision_;
    engine::ModelPreview model_preview_;
    std::vector<mesh::Surface> model_surfaces_;
    int scale_ = 1;
    bool imgui_ready_ = false;
    ui::LayoutEngine layout_;
    std::string settings_file_,settings_cache_;
    void save_settings();
    bool glyph_sample_ = false;
    ui::Viewport current_viewport_{};
    std::unordered_map<std::string, unsigned int> icon_textures_;
    unsigned int icon_texture(std::string_view name);
    std::unique_ptr<ui::Icons> design_icons_;
    std::unique_ptr<ui::GlViewport> gl_viewport_;
    std::vector<ui::GlViewport::Layer> viewport_layers_;
    std::array<std::pair<std::string,std::vector<engine::ModelProfile>>,2> sketch_fill_cache_;
    std::vector<mesh::Surface> display_surfaces_;
    std::string display_revision_,pick_camera_;
    unsigned design_icon_texture(std::string_view,ui::IconAccent,float size,bool disabled=false);
    void draw_design_board(const ui::Theme&);
    void draw();
    void draw_overlays(const ui::Theme& theme);
    void invoke_command(std::string_view id);
    void fit_camera();
    void sync_sketch();
    void draw_sketch_canvas(const ui::Theme&,bool hovered);
    void draw_sketch_panels(const ui::Theme&);
    void draw_timeline(const ui::Theme&);
    void draw_model_canvas(const ui::Theme&,bool hovered);
    void draw_model_panel(const ui::Theme&);
    void draw_geometry_canvas(const ui::Theme&,bool hovered);
    void draw_surface_panel(const ui::Theme&);
};
}
