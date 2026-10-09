#include "kernel/box_volume.hpp"
#include <BRepGProp.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <GProp_GProps.hxx>
#include <Standard_Failure.hxx>
#include <cmath>

namespace sagomacad::kernel {
std::optional<double> box_volume(double x, double y, double z) noexcept {
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z) || x <= 0 || y <= 0 || z <= 0)
        return std::nullopt;
    try {
        BRepPrimAPI_MakeBox builder(x, y, z);
        const auto& shape=builder.Shape();
        if (!builder.IsDone()) return std::nullopt;
        GProp_GProps properties;
        BRepGProp::VolumeProperties(shape, properties);
        const double volume=properties.Mass();
        if (!std::isfinite(volume)) return std::nullopt;
        return volume;
    } catch (const Standard_Failure&) {
        return std::nullopt;
    } catch (...) {
        return std::nullopt;
    }
}
}
