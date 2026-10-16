#pragma once
#include "engine/command.hpp"
#include "solver/profile_regions.hpp"
#include "solver/sketch_solver.hpp"
#include "mesh/surface.hpp"
namespace sagomacad::engine {
struct ModelProfile {
    solver::ProfileRegion region;
    kernel::Tessellation fill;
    kernel::Vec3 handle;
};
std::vector<ModelProfile> model_profiles(const solver::SketchResult&);
struct ModelPreview {
    std::optional<mesh::Surface> surface;
    std::string error;
    kernel::Vec3 handle,normal,axis_origin,axis_direction;
    std::vector<kernel::Vec3> angle_arc;
    double amount=0;
};
doc::Document model_context(const doc::Document&,const std::string& editing);
ModelPreview model_preview(const doc::Document&,const std::string& editing,const std::string& type,const nlohmann::json& params);
double model_drag_value(double start,double dx,double dy,double projected_x,double projected_y,bool angle);
}
