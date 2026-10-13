#include "mesh/surface.hpp"
#include <algorithm>

namespace sagomacad::mesh {
kernel::Result<Surface> build(const kernel::Shape& shape,double tolerance) noexcept {
    auto geometry=kernel::tessellate(shape,tolerance);
    if (!geometry) return kernel::Result<Surface>::failure(geometry.error);
    Surface result;
    result.geometry=std::move(*geometry.value);
    result.minimum=result.maximum=result.geometry.triangles.front().vertices.front();
    for (const auto& triangle:result.geometry.triangles)
        for (const auto& point:triangle.vertices) {
            result.minimum.x=std::min(result.minimum.x,point.x);
            result.minimum.y=std::min(result.minimum.y,point.y);
            result.minimum.z=std::min(result.minimum.z,point.z);
            result.maximum.x=std::max(result.maximum.x,point.x);
            result.maximum.y=std::max(result.maximum.y,point.y);
            result.maximum.z=std::max(result.maximum.z,point.z);
        }
    return kernel::Result<Surface>::success(std::move(result));
}
} // namespace sagomacad::mesh
