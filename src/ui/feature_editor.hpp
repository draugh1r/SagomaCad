#pragma once
#include "doc/document.hpp"
#include "ui/state.hpp"
#include <string_view>

namespace sagomacad::ui {
bool begin_feature_edit(State& state,const doc::Document& document,std::string_view feature_id);
doc::Document feature_preview(const State& state,const doc::Document& document);
}
