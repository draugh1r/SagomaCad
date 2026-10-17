#pragma once
#include "kernel/shape.hpp"
#include "doc/topology.hpp"
namespace sagomacad::kernel {
struct TopologyEntity {
    int id=0;
    std::string name;
    doc::GeometrySignature signature;
    std::vector<int> edges;
};
struct FaceFrame { Vec3 origin,u,v,normal; };
Result<Shape> name_topology(const Shape&,const std::string& origin) noexcept;
Result<std::vector<TopologyEntity>> topology(const Shape&) noexcept;
Result<TopologyEntity> resolve(const Shape&,const doc::TopologyRef&,const std::string& kind) noexcept;
Result<Shape> face_shape(const Shape&,int id) noexcept;
Result<FaceFrame> face_frame(const Shape&,int id) noexcept;
Result<Shape> fillet(const Shape&,const std::vector<int>& edges,double radius,const std::string& origin) noexcept;
Result<Shape> chamfer(const Shape&,const std::vector<int>& edges,double distance,const std::string& origin) noexcept;
}
