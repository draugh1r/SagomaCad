#include "ui/layout.hpp"
#include "ui/toolbar_catalog.hpp"
#include <clay.h>
#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cfloat>
#include <vector>

namespace sagomacad::ui {
namespace {
Clay_String str(std::string_view s) { return {true,static_cast<int32_t>(s.size()),s.data()}; }
Clay_ElementId id(std::string_view s) { return Clay_GetElementId(str(s)); }
Clay_Color color(Color c) { return {c.r*255,c.g*255,c.b*255,c.a*255}; }
float finite(float n,float fallback) { return std::isfinite(n)?n:fallback; }
uint16_t px(float n) { return static_cast<uint16_t>(std::clamp(std::round(n),0.0f,65535.0f)); }
Clay_Dimensions measure(Clay_StringSlice text,Clay_TextElementConfig* config,void*) {
    if (!ImGui::GetCurrentContext() || !ImGui::GetFont()) return {};
    const auto size=ImGui::GetFont()->CalcTextSizeA(static_cast<float>(config->fontSize),FLT_MAX,0,text.chars,text.chars+text.length);
    return {size.x,size.y};
}
LayoutRect rect(std::string_view name) {
    const auto data=Clay_GetElementData(id(name));
    if (!data.found) return {};
    const auto b=data.boundingBox;
    return {b.x,b.y,b.width,b.height};
}
bool valid(LayoutRect r,float w,float h) {
    constexpr float tolerance=0.1f;
    return std::isfinite(r.x)&&std::isfinite(r.y)&&std::isfinite(r.width)&&std::isfinite(r.height)&&
        r.x>=-tolerance&&r.y>=-tolerance&&r.width>=0&&r.height>=0&&r.x+r.width<=w+tolerance&&r.y+r.height<=h+tolerance;
}
}
struct LayoutEngine::Impl {
    Clay_Context* context=nullptr;
    std::vector<char> arena;
    ShellLayout last;
    std::string error;
    bool failed=false;
    // Render commands reference Clay's scratch memory. Copy them (including text) for fallback frames.
    struct Decoration { Clay_RenderCommand command; std::string text; };
    std::vector<Decoration> decorations;
    static void on_error(Clay_ErrorData data) {
        auto& self=*static_cast<Impl*>(data.userData);
        self.failed=true;
        self.error.assign(data.errorText.chars,data.errorText.length);
        std::fprintf(stderr,"Clay layout: %s\n",self.error.c_str());
    }
    explicit Impl(int element_limit) {
        auto* previous=Clay_GetCurrentContext();
        Clay_SetCurrentContext(nullptr);
        Clay_SetMaxElementCount(std::clamp(element_limit,32,max_elements));
        Clay_SetMaxMeasureTextCacheWordCount(max_text_words);
        arena.resize(Clay_MinMemorySize());
        context=Clay_Initialize(Clay_CreateArenaWithCapacityAndMemory(arena.size(),arena.data()),{}, {on_error,this});
        if (context) { Clay_SetMeasureTextFunction(measure,nullptr); Clay_SetCullingEnabled(false); }
        decorations.reserve(max_elements*2);
        Clay_SetCurrentContext(previous);
    }
};
LayoutEngine::LayoutEngine(int element_limit):impl_(std::make_unique<Impl>(element_limit)) {}
LayoutEngine::~LayoutEngine() { if (Clay_GetCurrentContext()==impl_->context) Clay_SetCurrentContext(nullptr); }
const ShellLayout& LayoutEngine::last() const { return impl_->last; }
const std::string& LayoutEngine::error() const { return impl_->error; }
const ShellLayout& LayoutEngine::compute(float w,float h,float z,const State& s,const Theme& t,bool glyph) {
    auto& self=*impl_;
    if (!self.context || !std::isfinite(w)||!std::isfinite(h)||!std::isfinite(z)||w<0||h<0||z<=0||z>8||w>65535||h>65535) {
        self.error="Invalid layout dimensions; keeping last valid layout";
        std::fprintf(stderr,"Clay layout: %s\n",self.error.c_str());
        return self.last;
    }
    auto* previous=Clay_GetCurrentContext();
    Clay_SetCurrentContext(self.context);
    self.failed=false; self.error.clear();
    Clay_SetLayoutDimensions({w,h});
    Clay_BeginLayout();
    const auto& a=t.layout;
    // Reserve the viewport first. Below its minimum, bars/panels shrink proportionally.
    const float header=std::min((t.toolbar_height-a.panel_border)*z,std::max(0.0f,h-a.viewport_min_height*z));
    const float top=std::min(a.topbar_height*z,header);
    const float divider=std::min(a.divider*z,std::max(0.0f,h-header)*0.25f);
    const float timeline=s.timeline?std::min(std::clamp(finite(s.timeline_height,t.timeline_height),a.timeline_min,a.timeline_max)*z,
        std::max(0.0f,h-header-divider*2-a.viewport_min_height*z)):0;
    const float browser=s.browser?(s.browser_collapsed?a.collapsed_width:std::clamp(finite(s.browser_width,t.sidebar_width-a.panel_border),a.browser_min,a.browser_max))*z:0;
    const float properties=s.properties?(s.properties_collapsed?a.collapsed_width:std::clamp(finite(s.properties_width,t.inspector_width-a.panel_border),a.properties_min,a.properties_max))*z:0;
    const float horizontal_divider=std::min(divider,w*0.25f);
    const float available=std::max(0.0f,w-a.viewport_min_width*z-(s.browser?horizontal_divider:0)-(s.properties?horizontal_divider:0));
    const float fraction=browser+properties>0?std::min(1.0f,available/(browser+properties)):0;
    const auto tabs=toolbar_catalog();
    const auto groups=tabs[std::clamp(s.selected_tab,0,static_cast<int>(tabs.size())-1)].groups;
    const bool ribbon_visible=header-top>=(a.ribbon_top+a.ribbon_bottom+a.group_button_height+a.group_spacer+a.group_menu_height)*z && w>=(t.padding+a.ribbon_right)*z;
    CLAY({.id=id("shell"),.layout={.sizing={CLAY_SIZING_FIXED(w),CLAY_SIZING_FIXED(h)},.layoutDirection=CLAY_TOP_TO_BOTTOM}}) {
        CLAY({.id=id("topbar"),.layout={.sizing={CLAY_SIZING_GROW(0),CLAY_SIZING_FIXED(top)}}}) {}
        CLAY({.id=id("ribbon"),.layout={.sizing={CLAY_SIZING_GROW(0),CLAY_SIZING_FIXED(header-top)},
            .padding={px(ribbon_visible?t.padding*z:0),px(ribbon_visible?a.ribbon_right*z:0),px(ribbon_visible?a.ribbon_top*z:0),px(ribbon_visible?a.ribbon_bottom*z:0)}},.clip={.horizontal=true,.vertical=true}}) {
            for (size_t i=0;ribbon_visible && i<groups.size() && i<self.last.groups.size();++i) {
                const auto& group=groups[i];
                const auto count=std::min({group.primary_count,group.commands.size(),size_t(4)});
                CLAY({.id=CLAY_IDI("group",static_cast<uint32_t>(i)),.layout={.sizing={CLAY_SIZING_GROW(0,t.ribbon_group_width*z),CLAY_SIZING_GROW(0)},.layoutDirection=CLAY_TOP_TO_BOTTOM},
                    .border={.color=color(t.border),.width={.right=px(a.panel_border*z)}}}) {
                    CLAY({.layout={.sizing={CLAY_SIZING_GROW(0),CLAY_SIZING_FIXED(a.group_button_height*z)},.padding={px(a.group_left*z),px(a.group_right*z)}}}) {
                        for (size_t j=0;j<count;++j) {
                            CLAY({.id=CLAY_IDI("ribbon_button",static_cast<uint32_t>(i*4+j)),.layout={.sizing={CLAY_SIZING_GROW(0),CLAY_SIZING_GROW(0)}}}) {}
                        }
                    }
                    CLAY({.layout={.sizing={CLAY_SIZING_GROW(0),CLAY_SIZING_FIXED(a.group_spacer*z)}}}) {}
                    CLAY({.layout={.sizing={CLAY_SIZING_GROW(0),CLAY_SIZING_FIXED(a.group_menu_height*z)},.padding={px(a.menu_left*z),px(a.menu_right*z)}}}) {
                        CLAY({.id=CLAY_IDI("group_menu",static_cast<uint32_t>(i)),.layout={.sizing={CLAY_SIZING_GROW(0),CLAY_SIZING_GROW(0)},
                            .padding={.right=px(a.label_right*z),.top=px(a.label_top*z)},.childAlignment={.x=CLAY_ALIGN_X_CENTER}}}) {
                            CLAY_TEXT(str(group.label),CLAY_TEXT_CONFIG({.textColor=color(t.muted),.fontSize=px(a.label_font_size*z),.wrapMode=CLAY_TEXT_WRAP_NONE}));
                        }
                    }
                }
            }
        }
        CLAY({.id=id("header_separator"),.layout={.sizing={CLAY_SIZING_GROW(0),CLAY_SIZING_FIXED(divider)}},.backgroundColor=color(t.border)}) {}
        CLAY({.layout={.sizing={CLAY_SIZING_GROW(0),CLAY_SIZING_GROW(0)}}}) {
            if (s.browser) {
                CLAY({.id=id("browser"),.layout={.sizing={CLAY_SIZING_FIXED(browser*fraction),CLAY_SIZING_GROW(0)}}}) {}
                CLAY({.id=id("browser_separator"),.layout={.sizing={CLAY_SIZING_FIXED(horizontal_divider),CLAY_SIZING_GROW(0)}},.backgroundColor=color(t.border)}) {}
            }
            CLAY({.id=id("viewport"),.layout={.sizing={CLAY_SIZING_GROW(0),CLAY_SIZING_GROW(0)}}}) {}
            if (s.properties) {
                CLAY({.id=id("properties_separator"),.layout={.sizing={CLAY_SIZING_FIXED(horizontal_divider),CLAY_SIZING_GROW(0)}},.backgroundColor=color(t.border)}) {}
                CLAY({.id=id("properties"),.layout={.sizing={CLAY_SIZING_FIXED(properties*fraction),CLAY_SIZING_GROW(0)}}}) {}
            }
            // Reserved for future contextual areas; existing sketch overlays remain unchanged.
            CLAY({.id=id("contextual"),.layout={.sizing={CLAY_SIZING_FIXED(0),CLAY_SIZING_GROW(0)}}}) {}
        }
        if (s.timeline) {
            CLAY({.id=id("timeline_separator"),.layout={.sizing={CLAY_SIZING_GROW(0),CLAY_SIZING_FIXED(divider)}},.backgroundColor=color(t.border)}) {}
            const float th=glyph?std::min(t.timeline_glyph_height*z,std::max(0.0f,h-header-divider*2-a.viewport_min_height*z)):timeline;
            CLAY({.id=id("timeline"),.layout={.sizing={CLAY_SIZING_GROW(0),CLAY_SIZING_FIXED(th)}}}) {}
        }
    }
    auto commands=Clay_EndLayout();
    // Clay 0.14 reports element exhaustion as a diagnostic render command rather
    // than invoking its error callback. Route that case through the same handler.
    for (int i=0;i<commands.length;++i) {
        const auto& c=*Clay_RenderCommandArray_Get(&commands,i);
        if (c.commandType!=CLAY_RENDER_COMMAND_TYPE_TEXT) continue;
        const auto text=c.renderData.text.stringContents;
        if (std::string_view(text.chars,text.length).starts_with("Clay Error:"))
            Impl::on_error({CLAY_ERROR_TYPE_ELEMENTS_CAPACITY_EXCEEDED,{true,text.length,text.chars},&self});
    }
    ShellLayout candidate;
    candidate.topbar=rect("topbar"); candidate.ribbon=rect("ribbon"); candidate.browser=s.browser?rect("browser"):LayoutRect{};
    candidate.viewport=rect("viewport"); candidate.properties=s.properties?rect("properties"):LayoutRect{};
    candidate.timeline=s.timeline?rect("timeline"):LayoutRect{}; candidate.contextual=rect("contextual");
    candidate.header_separator=rect("header_separator"); candidate.browser_separator=s.browser?rect("browser_separator"):LayoutRect{};
    candidate.properties_separator=s.properties?rect("properties_separator"):LayoutRect{}; candidate.timeline_separator=s.timeline?rect("timeline_separator"):LayoutRect{};
    candidate.group_count=ribbon_visible?std::min(groups.size(),candidate.groups.size()):0;
    for (size_t i=0;i<candidate.group_count;++i) {
        auto& g=candidate.groups[i];
        auto box=Clay_GetElementData(CLAY_IDI("group",static_cast<uint32_t>(i))).boundingBox;
        g.group={box.x,box.y,box.width,box.height};
        box=Clay_GetElementData(CLAY_IDI("group_menu",static_cast<uint32_t>(i))).boundingBox;
        g.menu={box.x,box.y,box.width,box.height};
        g.button_count=std::min({groups[i].primary_count,groups[i].commands.size(),g.buttons.size()});
        for (size_t j=0;j<g.button_count;++j) {
            box=Clay_GetElementData(CLAY_IDI("ribbon_button",static_cast<uint32_t>(i*4+j))).boundingBox;
            g.buttons[j]={box.x,box.y,box.width,box.height};
        }
    }
    for (auto r:{candidate.topbar,candidate.ribbon,candidate.browser,candidate.viewport,candidate.properties,candidate.timeline,candidate.contextual})
        if (!valid(r,w,h)) {
            self.failed=true; self.error="Invalid Clay rectangle; keeping last valid layout";
            std::fprintf(stderr,"Clay invalid rectangle: %g %g %g %g in %g %g\n",r.x,r.y,r.width,r.height,w,h);
        }
    if (!self.failed) {
        self.last=candidate;
        self.decorations.clear();
        for (int i=0;i<commands.length;++i) {
            const auto c=*Clay_RenderCommandArray_Get(&commands,i);
            Impl::Decoration d{c,{}};
            if (c.commandType==CLAY_RENDER_COMMAND_TYPE_TEXT) d.text.assign(c.renderData.text.stringContents.chars,c.renderData.text.stringContents.length);
            self.decorations.push_back(std::move(d));
        }
    }
    Clay_SetCurrentContext(previous);
    return self.last;
}
void LayoutEngine::render(ImDrawList* dl) const {
    auto packed=[](Clay_Color c) { return IM_COL32(static_cast<int>(c.r),static_cast<int>(c.g),static_cast<int>(c.b),static_cast<int>(c.a)); };
    int clips=0;
    for (const auto& d:impl_->decorations) {
        const auto& c=d.command; const auto& b=c.boundingBox;
        const ImVec2 lo{b.x,b.y},hi{b.x+b.width,b.y+b.height};
        switch (c.commandType) {
        case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: dl->AddRectFilled(lo,hi,packed(c.renderData.rectangle.backgroundColor),c.renderData.rectangle.cornerRadius.topLeft); break;
        case CLAY_RENDER_COMMAND_TYPE_TEXT: dl->AddText(ImGui::GetFont(),static_cast<float>(c.renderData.text.fontSize),lo,packed(c.renderData.text.textColor),d.text.data(),d.text.data()+d.text.size()); break;
        case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: dl->PushClipRect(lo,hi,true); ++clips; break;
        case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: if (clips) { dl->PopClipRect(); --clips; } break;
        case CLAY_RENDER_COMMAND_TYPE_BORDER: {
            const auto& border=c.renderData.border; const auto col=packed(border.color);
            if (border.width.left) dl->AddLine(lo,{lo.x,hi.y},col,static_cast<float>(border.width.left));
            if (border.width.right) dl->AddLine({hi.x,lo.y},hi,col,static_cast<float>(border.width.right));
            if (border.width.top) dl->AddLine(lo,{hi.x,lo.y},col,static_cast<float>(border.width.top));
            if (border.width.bottom) dl->AddLine({lo.x,hi.y},hi,col,static_cast<float>(border.width.bottom));
            break;
        }
        default: break;
        }
    }
    while (clips-->0) dl->PopClipRect();
}
}
