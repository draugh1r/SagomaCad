#pragma once
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <array>
#include <vector>

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
struct Triangle { std::array<Vec3,3> vertices; Vec3 normal; int face_id=0; };
struct Polyline { std::vector<Vec3> points; };
struct Tessellation { std::vector<Triangle> triangles; std::vector<Polyline> edges; };

class Shape {
public:
    Shape() = default;
    bool valid() const { return static_cast<bool>(impl_); }
private:
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
};

Result<Shape> make_box(double width,double depth,double height) noexcept;
Result<Shape> make_cylinder(double radius,double height) noexcept;
Result<Shape> make_sphere(double radius) noexcept;
Result<MassProperties> mass_properties(const Shape& shape) noexcept;
Result<Shape> translate(const Shape& shape,double x,double y,double z) noexcept;
Result<Shape> subtract(const Shape& target,const Shape& cutter) noexcept;
Result<Tessellation> tessellate(const Shape& shape,double tolerance) noexcept;

} // namespace sagomacad::kernel
