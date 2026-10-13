#pragma once
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace sagomacad::kernel {

template<class T> struct Result {
    std::optional<T> value;
    std::string error;
    explicit operator bool() const { return value.has_value(); }
    static Result success(T result) { return {std::move(result), {}}; }
    static Result failure(std::string message) { return {std::nullopt, std::move(message)}; }
};

struct MassProperties { double volume=0.0; double area=0.0; };

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
};

Result<Shape> make_box(double width,double depth,double height) noexcept;
Result<Shape> make_cylinder(double radius,double height) noexcept;
Result<Shape> make_sphere(double radius) noexcept;
Result<MassProperties> mass_properties(const Shape& shape) noexcept;

} // namespace sagomacad::kernel
