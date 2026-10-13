#pragma once
#include "kernel/shape.hpp"
#include <filesystem>

namespace sagomacad::mesh {
struct Surface {
    kernel::Tessellation geometry;
    kernel::Vec3 minimum;
    kernel::Vec3 maximum;
};
kernel::Result<Surface> build(const kernel::Shape& shape,double tolerance) noexcept;
kernel::Result<std::size_t> write_stl(const Surface& surface,const std::filesystem::path& path) noexcept;
} // namespace sagomacad::mesh
