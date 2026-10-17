#pragma once
#include "regen/regenerator.hpp"
#include "doc/sketch.hpp"
namespace sagomacad::regen {
doc::Sketch attached_sketch(doc::Sketch,const Snapshot&);
doc::Sketch evaluated_sketch(const doc::Document&,const std::string& id);
}
