#pragma once
#include "regen/regenerator.hpp"
namespace sagomacad::regen {
Regenerator::Cached build_model_feature(const doc::Document&,const doc::Feature&,const Snapshot&);
}
