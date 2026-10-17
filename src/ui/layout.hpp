#pragma once
#include "ui/state.hpp"
#include "ui/theme.hpp"
#include <array>
#include <memory>
#include <string>
#include <string_view>

struct ImDrawList;
namespace sagomacad::ui {
struct LayoutRect {
    float x=0, y=0, width=0, height=0;
    bool visible() const { return width>0 && height>0; }
};
struct RibbonLayout {
    LayoutRect group, menu;
    std::array<LayoutRect,4> buttons{};
    size_t button_count=0;
};
struct ShellLayout {
    LayoutRect topbar, ribbon, browser, viewport, properties, timeline, contextual;
    LayoutRect sketch_palette;
    LayoutRect header_separator, browser_separator, properties_separator, timeline_separator;
    std::array<RibbonLayout,8> groups{};
    size_t group_count=0;
    int tab_index=0;
};
// Owns one bounded Clay arena; no Clay types escape ui/.
class LayoutEngine {
public:
    static constexpr int max_elements=512, max_text_words=1024;
    explicit LayoutEngine(int element_limit=max_elements);
    ~LayoutEngine();
    LayoutEngine(const LayoutEngine&)=delete;
    const ShellLayout& compute(float width, float height, float scale, const State&, const Theme&, bool glyph_sample=false);
    const ShellLayout& last() const;
    const std::string& error() const;
    void render(ImDrawList*) const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
