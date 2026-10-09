#pragma once
#include "ui/state.hpp"
#include <SDL3/SDL_video.h>
#include <nlohmann/json.hpp>
#include <string>

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
    bool screenshot(const std::string& path, std::string& error);
    void set_glyph_sample(bool enabled) { glyph_sample_ = enabled; }
    bool glyphs_available() const;
    nlohmann::json inspect() const;
    ui::State& state() { return state_; }
private:
    SDL_Window* window_ = nullptr;
    SDL_GLContext context_ = nullptr;
    std::string error_;
    ui::State state_;
    int scale_ = 1;
    bool imgui_ready_ = false;
    bool glyph_sample_ = false;
    void draw();
};
}
