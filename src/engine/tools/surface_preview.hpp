#pragma once
#include "engine/command.hpp"
#include "mesh/surface.hpp"
namespace sagomacad::engine {
struct SurfaceDimension {kernel::Vec3 from,to,direction;double value=0;};
struct SurfacePreview {
    std::optional<mesh::Surface> surface;
    std::string source_body;
    kernel::Tessellation cutter;
    nlohmann::json params;
    std::string error;
    kernel::Vec3 start,handle,normal;
    double amount=0;
    std::vector<SurfaceDimension> dimensions;
};
SurfacePreview surface_preview(const doc::Document&,const std::string& editing,const std::string& command,const nlohmann::json& params);
kernel::Result<nlohmann::json> face_position(const doc::Document&,const nlohmann::json& ref,kernel::Vec3 ray_origin,kernel::Vec3 ray_direction) noexcept;
}
