#include "ui/layout.hpp"
#include <nlohmann/json.hpp>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#define require(c) do { if (!(c)) { std::fprintf(stderr,"line %d: %s\n",__LINE__,#c); std::exit(1); } } while(false)
using namespace sagomacad::ui;
bool overlap(LayoutRect a,LayoutRect b) {
    return a.visible()&&b.visible()&&a.x<b.x+b.width-0.01f&&b.x<a.x+a.width-0.01f&&a.y<b.y+b.height-0.01f&&b.y<a.y+a.height-0.01f;
}
void verify(const ShellLayout& l,float w,float h) {
    const std::array r{l.topbar,l.ribbon,l.browser,l.viewport,l.properties,l.timeline,l.contextual};
    for (size_t i=0;i<r.size();++i) {
        const auto a=r[i];
        require(std::isfinite(a.x)&&std::isfinite(a.y)&&std::isfinite(a.width)&&std::isfinite(a.height));
        require(a.width>=0&&a.height>=0&&a.x>=0&&a.y>=0);
        require(a.x+a.width<=w+0.1f&&a.y+a.height<=h+0.1f);
        for (size_t j=i+1;j<r.size();++j) require(!overlap(a,r[j]));
    }
}
int main() {
    LayoutEngine layout,other;
    State s;
    for (auto size:{std::pair{1280,720}, {1920,1080},{800,600},{320,240},{100,60},{0,0}})
        for (int mask=0;mask<32;++mask) for (float scale:{1.0f,2.0f}) {
            s.browser=mask&1; s.properties=mask&2; s.timeline=mask&4;
            s.browser_collapsed=mask&8; s.properties_collapsed=mask&16;
            s.browser_width=480; s.properties_width=480; s.timeline_height=240;
            const auto& l=layout.compute(size.first*scale,size.second*scale,scale,s,light);
            if (!layout.error().empty()) std::fprintf(stderr,"size %d x %d mask %d scale %.0f: %s\n",size.first,size.second,mask,scale,layout.error().c_str());
            require(layout.error().empty());
            verify(l,size.first*scale,size.second*scale);
            if (size.first>=800) { require(l.viewport.width>=light.layout.viewport_min_width*scale-0.1f); require(l.viewport.height>=light.layout.viewport_min_height*scale-0.1f); }
        }
    s=State{};
    auto first=layout.compute(1280,720,1,s,dark);
    require(layout.error().empty());
    other.compute(800,600,1,s,light);
    const auto again=layout.compute(1280,720,1,s,dark);
    require(first.viewport.x==again.viewport.x&&first.viewport.width==again.viewport.width);
    const auto fallback=layout.compute(std::numeric_limits<float>::quiet_NaN(),720,1,s,dark);
    require(!layout.error().empty()); require(fallback.viewport.width==again.viewport.width);
    // A real Clay capacity error invokes our handler and retains the valid empty-tab layout.
    LayoutEngine constrained(32);
    s.selected_tab=2;
    const auto before_capacity=constrained.compute(800,600,1,s,dark);
    require(constrained.error().empty());
    s.selected_tab=0;
    const auto after_capacity=constrained.compute(800,600,1,s,dark);
    require(!constrained.error().empty());
    require(after_capacity.group_count==before_capacity.group_count&&after_capacity.viewport.width==before_capacity.viewport.width);
    s.selected_tab=2;
    constrained.compute(800,600,1,s,dark); require(constrained.error().empty());
    s.browser_width=333; s.properties_width=301; s.timeline_height=140; s.browser_collapsed=true; s.properties=true;
    State restored;
    restore_layout_state(restored,nlohmann::json::parse(layout_state_json(s).dump()));
    require(restored.browser_width==333&&restored.properties_width==301&&restored.timeline_height==140&&restored.browser_collapsed&&restored.properties);
    restore_layout_state(restored,{{"browser_width",-100},{"properties_width",1e300},{"timeline_height","wrong"},{"browser","wrong"}});
    require(restored.browser_width==light.layout.browser_min&&restored.properties_width==light.layout.properties_max&&restored.timeline_height==140);
    restore_layout_state(restored,nlohmann::json::array());
}
