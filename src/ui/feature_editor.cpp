#include "ui/feature_editor.hpp"
#include <algorithm>
#include <cstdio>

namespace sagomacad::ui {
namespace {
std::array<const char*,3> fields(const doc::Feature& feature) {
    if (feature.type=="primitive.box") return {"width","depth","height"};
    if (feature.type=="primitive.cylinder") return {"radius","height",nullptr};
    if (feature.type=="primitive.sphere") return {"radius",nullptr,nullptr};
    return {nullptr,nullptr,nullptr};
}
}
bool begin_feature_edit(State& state,const doc::Document& document,std::string_view id) {
    const auto found=document.features.find(std::string(id));
    if (found==document.features.end() || !found->second.data.is_object() ||
        !found->second.data.contains("dimensions") || !found->second.data["dimensions"].is_object()) return false;
    const auto keys=fields(found->second);
    if (!keys[0]) return false;
    const auto& dimensions=found->second.data["dimensions"];
    for (int i=0;i<3;++i) {
        state.primitive_dimensions[i].fill('\0');
        if (keys[i]) {
            if (!dimensions.contains(keys[i]) || !dimensions[keys[i]].is_string()) return false;
            const auto value=dimensions[keys[i]].get<std::string>();
            if (value.size()>=state.primitive_dimensions[i].size()) return false;
            std::snprintf(state.primitive_dimensions[i].data(),state.primitive_dimensions[i].size(),"%s",value.c_str());
        }
    }
    state.primitive_feature_id=std::string(id);
    state.primitive_error.clear(); state.primitive_preview_error.clear();
    state.primitive_create_open=false;
    state.primitive_editor_open=true;
    return true;
}
doc::Document feature_preview(const State& state,const doc::Document& document) {
    if (!state.primitive_editor_open) return document;
    auto preview=document;
    const auto found=preview.features.find(state.primitive_feature_id);
    if (found==preview.features.end() || !found->second.data.is_object()) return document;
    const auto keys=fields(found->second);
    if (!keys[0]) return document;
    auto& dimensions=found->second.data["dimensions"];
    for (int i=0;i<3;++i) if (keys[i]) dimensions[keys[i]]=state.primitive_dimensions[i].data();
    const auto at=std::find(preview.timeline.begin(),preview.timeline.end(),state.primitive_feature_id);
    if (at!=preview.timeline.end()) preview.rollback=std::size_t(at-preview.timeline.begin())+1;
    return preview;
}
}
