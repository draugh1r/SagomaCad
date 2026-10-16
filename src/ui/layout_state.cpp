#include "ui/state.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>

namespace sagomacad::ui {
nlohmann::json layout_state_json(const State& s) {
    return {{"version",1},{"browser",s.browser},{"properties",s.properties},{"timeline",s.timeline},
        {"browser_collapsed",s.browser_collapsed},{"properties_collapsed",s.properties_collapsed},
        {"browser_width",s.browser_width},{"properties_width",s.properties_width},{"timeline_height",s.timeline_height}};
}
void restore_layout_state(State& s,const nlohmann::json& j) {
    if (!j.is_object()) return;
    const auto& t=theme(s.theme=="light");
    auto number=[&](const char* key,float& value,float lo,float hi) {
        const auto it=j.find(key);
        if (it==j.end() || !it->is_number()) return;
        const double n=it->get<double>();
        if (std::isfinite(n)) value=static_cast<float>(std::clamp(n,static_cast<double>(lo),static_cast<double>(hi)));
    };
    auto flag=[&](const char* key,bool& value) { const auto it=j.find(key); if (it!=j.end()&&it->is_boolean()) value=it->get<bool>(); };
    number("browser_width",s.browser_width,t.layout.browser_min,t.layout.browser_max);
    number("properties_width",s.properties_width,t.layout.properties_min,t.layout.properties_max);
    number("timeline_height",s.timeline_height,t.layout.timeline_min,t.layout.timeline_max);
    flag("browser",s.browser); flag("properties",s.properties); flag("timeline",s.timeline);
    flag("browser_collapsed",s.browser_collapsed); flag("properties_collapsed",s.properties_collapsed);
}
}
