#pragma once
#include "kernel/topology.hpp"
namespace sagomacad::kernel {
struct Measurement { double distance=0; std::optional<double> angle_degrees; };
Result<Measurement> measure(const Shape&,const TopologyEntity&,const Shape&,const TopologyEntity&) noexcept;
}
