#include "mesh/surface.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <limits>

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
kernel::Result<std::size_t> write_stl(const Surface& surface,const std::filesystem::path& path) noexcept {
    try {
        if (surface.geometry.triangles.empty() || surface.geometry.triangles.size()>std::numeric_limits<std::uint32_t>::max())
            return kernel::Result<std::size_t>::failure("Mesh STL vuota o troppo grande");
        const auto temporary=path.string()+".tmp";
        std::ofstream output(temporary,std::ios::binary|std::ios::trunc);
        if (!output) return kernel::Result<std::size_t>::failure("Impossibile aprire il file STL");
        std::array<char,80> header{};
        const char signature[]="SagomaCad binary STL";
        std::copy(signature,signature+sizeof(signature)-1,header.begin());
        output.write(header.data(),header.size());
        const auto count=static_cast<std::uint32_t>(surface.geometry.triangles.size());
        output.write(reinterpret_cast<const char*>(&count),sizeof(count));
        auto coord=[&](double value) { const float v=static_cast<float>(value); output.write(reinterpret_cast<const char*>(&v),sizeof(v)); };
        for (const auto& tri:surface.geometry.triangles) {
            coord(tri.normal.x);coord(tri.normal.y);coord(tri.normal.z);
            for (const auto& p:tri.vertices) { coord(p.x);coord(p.y);coord(p.z); }
            const std::uint16_t attributes=0;
            output.write(reinterpret_cast<const char*>(&attributes),sizeof(attributes));
        }
        output.close();
        if (!output) { std::filesystem::remove(temporary); return kernel::Result<std::size_t>::failure("Scrittura STL fallita"); }
        if (std::filesystem::exists(path)) std::filesystem::remove(path);
        std::filesystem::rename(temporary,path);
        return kernel::Result<std::size_t>::success(surface.geometry.triangles.size());
    } catch (const std::exception& e) { return kernel::Result<std::size_t>::failure(e.what()); }
      catch (...) { return kernel::Result<std::size_t>::failure("Errore imprevisto durante l'export STL"); }
}
} // namespace sagomacad::mesh
