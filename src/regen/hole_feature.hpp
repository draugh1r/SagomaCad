#pragma once
#include "regen/regenerator.hpp"
#include "kernel/topology.hpp"
namespace sagomacad::regen {
struct HolePlacement { double x=0,y=0; kernel::FaceFrame frame; kernel::TopologyEntity face; };
kernel::Result<HolePlacement> hole_placement(const doc::Document&,const nlohmann::json&,const kernel::Shape&) noexcept;
Regenerator::Cached build_hole_feature(const doc::Document&,const doc::Feature&,const Snapshot&);
}
