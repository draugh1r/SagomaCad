#pragma once
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <array>
#include <vector>
#include "doc/sketch.hpp"

namespace sagomacad::kernel {

template<class T> struct Result {
    std::optional<T> value;
    std::string error;
    explicit operator bool() const { return value.has_value(); }
    static Result success(T result) { return {std::move(result), {}}; }
    static Result failure(std::string message) { return {std::nullopt, std::move(message)}; }
};

struct Vec3 { double x=0.0,y=0.0,z=0.0; };
struct MassProperties { double volume=0.0; double area=0.0; Vec3 center; Vec3 minimum; Vec3 maximum; };
struct Triangle { std::array<Vec3,3> vertices; Vec3 normal; int face_id=0; std::array<Vec3,3> normals{}; };
struct Polyline { std::vector<Vec3> points; int edge_id=0; };
struct Tessellation { std::vector<Triangle> triangles; std::vector<Polyline> edges; };

class Shape {
public:
    Shape() = default;
    bool valid() const { return static_cast<bool>(impl_); }
private:
    friend struct ShapeAccess;
    struct Impl;
    std::shared_ptr<Impl> impl_;
    explicit Shape(std::shared_ptr<Impl> impl) : impl_(std::move(impl)) {}
    friend Result<Shape> make_box(double,double,double) noexcept;
    friend Result<Shape> make_cylinder(double,double) noexcept;
    friend Result<Shape> make_sphere(double) noexcept;
    friend Result<MassProperties> mass_properties(const Shape&) noexcept;
    friend Result<Shape> translate(const Shape&,double,double,double) noexcept;
    friend Result<Shape> subtract(const Shape&,const Shape&) noexcept;
    friend Result<Tessellation> tessellate(const Shape&,double) noexcept;
    friend Result<Shape> profile_face(const doc::Sketch&,const std::vector<std::string>&,const std::vector<std::vector<std::string>>&) noexcept;
    friend Result<Shape> extrude(const Shape&,Vec3) noexcept;
    friend Result<Shape> revolve(const Shape&,Vec3,Vec3,double) noexcept;
    friend Result<Shape> boolean_operation(const Shape&,const Shape&,const std::string&) noexcept;
};

Result<Shape> make_box(double width,double depth,double height) noexcept;
Result<Shape> make_cylinder(double radius,double height) noexcept;
Result<Shape> make_sphere(double radius) noexcept;
Result<MassProperties> mass_properties(const Shape& shape) noexcept;
Result<Shape> translate(const Shape& shape,double x,double y,double z) noexcept;
Result<Shape> subtract(const Shape& target,const Shape& cutter) noexcept;
Result<Tessellation> tessellate(const Shape& shape,double tolerance) noexcept;
Result<Shape> profile_face(const doc::Sketch&,const std::vector<std::string>& boundary,const std::vector<std::vector<std::string>>& holes) noexcept;
Result<Shape> extrude(const Shape& face,Vec3 vector) noexcept;
Result<Shape> revolve(const Shape& face,Vec3 origin,Vec3 direction,double radians) noexcept;
Result<Shape> boolean_operation(const Shape&,const Shape&,const std::string& operation) noexcept;

} // namespace sagomacad::kernel
