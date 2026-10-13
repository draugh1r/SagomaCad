#pragma once
#include "doc/document.hpp"
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace sagomacad::doc {

enum class Dimension { Scalar, Length, Angle };
struct Quantity {
    double value = 0.0; // mm for length, radians for angle
    Dimension dimension = Dimension::Scalar;
};
struct EvaluatedParameter {
    std::string name;
    std::string expression;
    std::string unit;
    std::optional<double> value;
    std::string error;
};

std::vector<EvaluatedParameter> evaluate_parameters(const Document& document);
bool evaluate_expression(const Document& document, std::string_view expression,
                         Quantity& result, std::string& error);
std::string default_unit(Dimension dimension);

} // namespace sagomacad::doc
