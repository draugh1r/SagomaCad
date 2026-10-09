#pragma once
#include <optional>

namespace sagomacad::kernel {
std::optional<double> box_volume(double x, double y, double z) noexcept;
}
