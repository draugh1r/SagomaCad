#pragma once
#include "kernel/shape.hpp"

namespace sagomacad::mesh {
struct Surface {
    kernel::Tessellation geometry;
    kernel::Vec3 minimum;
    kernel::Vec3 maximum;
};
kernel::Result<Surface> build(const kernel::Shape& shape,double tolerance) noexcept;
} // namespace sagomacad::mesh
