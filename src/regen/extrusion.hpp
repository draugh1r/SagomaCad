#pragma once
#include "regen/regenerator.hpp"
namespace sagomacad::regen {
kernel::Result<kernel::Shape> sweep_profile(const doc::Document&,const nlohmann::json&,const Snapshot&,kernel::Shape face,kernel::Vec3 normal,kernel::Vec3 origin) noexcept;
}
