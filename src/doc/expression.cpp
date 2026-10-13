#include "doc/expression.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace sagomacad::doc {
namespace {
struct Unit { Dimension dimension; double scale; };
const std::unordered_map<std::string, Unit> units = {
    {"mm", {Dimension::Length, 1.0}}, {"cm", {Dimension::Length, 10.0}},
    {"m", {Dimension::Length, 1000.0}}, {"in", {Dimension::Length, 25.4}},
    {"ft", {Dimension::Length, 304.8}},
    {"rad", {Dimension::Angle, 1.0}}, {"deg", {Dimension::Angle, 0.017453292519943295}},
};

bool ident_start(unsigned char c) { return c == '_' || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c >= 128; }
bool ident_rest(unsigned char c) { return ident_start(c) || (c >= '0' && c <= '9'); }

class Resolver;
class Parser {
public:
    Parser(std::string_view input, Resolver& resolver) : input_(input), resolver_(resolver) {}
    Quantity parse();
private:
    std::string_view input_;
    Resolver& resolver_;
    std::size_t pos_ = 0;
    int depth_ = 0;
    void spaces();
    bool take(char c);
    Quantity sum();
    Quantity product();
    Quantity unary();
    Quantity primary();
    std::string identifier();
};

class Resolver {
public:
    explicit Resolver(const Document& document) : document_(document), states_(document.parameters.size()) {
        for (std::size_t i = 0; i < document.parameters.size(); ++i)
            if (!indices_.emplace(document.parameters[i].name, i).second)
                duplicates_.insert(document.parameters[i].name);
    }
    Quantity resolve(const std::string& name) {
        if (duplicates_.contains(name)) throw std::runtime_error("Nome di parametro duplicato: " + name);
        const auto it = indices_.find(name);
        if (it == indices_.end()) throw std::runtime_error("Parametro sconosciuto: " + name);
        const auto index = it->second;
        auto& state = states_[index];
        if (state.done) {
            if (!state.error.empty()) throw std::runtime_error(state.error);
            return state.quantity;
        }
        if (state.visiting) {
            const auto first = std::find(stack_.begin(), stack_.end(), name);
            std::string chain = "Riferimento circolare: ";
            for (auto member = first; member != stack_.end(); ++member) {
                if (member != first) chain += " -> ";
                chain += *member;
            }
            chain += " -> " + name;
            throw std::runtime_error(chain);
        }
        if (stack_.size() >= 64) throw std::runtime_error("Troppi riferimenti tra parametri");
        state.visiting = true;
        stack_.push_back(name);
        try {
            state.quantity = Parser(document_.parameters[index].expression, *this).parse();
            const auto& declared = document_.parameters[index].unit;
            if (!declared.empty()) {
                const auto unit = units.find(declared);
                if (unit == units.end()) throw std::runtime_error("Unità sconosciuta: " + declared);
                if (state.quantity.dimension == Dimension::Scalar) {
                    state.quantity.value *= unit->second.scale;
                    state.quantity.dimension = unit->second.dimension;
                    if (!std::isfinite(state.quantity.value))
                        throw std::runtime_error("Risultato numerico non finito");
                }
                if (unit->second.dimension != state.quantity.dimension)
                    throw std::runtime_error("Unità incompatibili nel parametro " + name);
            }
        } catch (const std::exception& e) {
            state.error = e.what();
        }
        stack_.pop_back();
        state.visiting = false;
        state.done = true;
        if (!state.error.empty()) throw std::runtime_error(state.error);
        return state.quantity;
    }
    Quantity expression(std::string_view value) { return Parser(value, *this).parse(); }
private:
    struct State { bool visiting = false; bool done = false; Quantity quantity; std::string error; };
    const Document& document_;
    std::unordered_map<std::string, std::size_t> indices_;
    std::unordered_set<std::string> duplicates_;
    std::vector<State> states_;
    std::vector<std::string> stack_;
};

void Parser::spaces() {
    while (pos_ < input_.size() && (input_[pos_] == ' ' || input_[pos_] == '\t' || input_[pos_] == '\n')) ++pos_;
}
bool Parser::take(char c) {
    spaces();
    if (pos_ < input_.size() && input_[pos_] == c) { ++pos_; return true; }
    return false;
}
std::string Parser::identifier() {
    const auto start = pos_++;
    while (pos_ < input_.size() && ident_rest(static_cast<unsigned char>(input_[pos_]))) ++pos_;
    return std::string(input_.substr(start, pos_ - start));
}
Quantity Parser::primary() {
    if (++depth_ > 64) throw std::runtime_error("Espressione troppo annidata");
    spaces();
    Quantity value;
    if (take('(')) {
        value = sum();
        if (!take(')')) throw std::runtime_error("Manca la parentesi chiusa");
    } else if (pos_ < input_.size() &&
               ((input_[pos_] >= '0' && input_[pos_] <= '9') || input_[pos_] == '.')) {
        double number = 0.0;
        const char* begin = input_.data() + pos_;
        const char* end = input_.data() + input_.size();
        const auto parsed = std::from_chars(begin, end, number, std::chars_format::general);
        if (parsed.ec != std::errc{} || parsed.ptr == begin || !std::isfinite(number))
            throw std::runtime_error("Numero non valido o troppo grande");
        pos_ += static_cast<std::size_t>(parsed.ptr - begin);
        value.value = number;
        spaces();
        if (pos_ < input_.size() && ident_start(static_cast<unsigned char>(input_[pos_]))) {
            const std::string symbol = identifier();
            const auto unit = units.find(symbol);
            if (unit == units.end()) throw std::runtime_error("Unità sconosciuta: " + symbol);
            value.dimension = unit->second.dimension;
            value.value *= unit->second.scale;
        }
    } else if (pos_ < input_.size() && ident_start(static_cast<unsigned char>(input_[pos_]))) {
        value = resolver_.resolve(identifier());
    } else {
        throw std::runtime_error("Valore atteso nell'espressione");
    }
    --depth_;
    if (!std::isfinite(value.value)) throw std::runtime_error("Risultato numerico non finito");
    return value;
}
Quantity Parser::unary() {
    bool negative = false;
    for (int count = 0; count < 64; ++count) {
        if (take('+')) continue;
        if (take('-')) { negative = !negative; continue; }
        auto value = primary();
        if (negative) value.value = -value.value;
        return value;
    }
    throw std::runtime_error("Troppi segni unari nell'espressione");
}
Quantity Parser::product() {
    auto left = unary();
    while (true) {
        if (take('*')) {
            const auto right = unary();
            if (left.dimension != Dimension::Scalar && right.dimension != Dimension::Scalar)
                throw std::runtime_error("Unità incompatibili nella moltiplicazione");
            left = {left.value * right.value,
                    left.dimension == Dimension::Scalar ? right.dimension : left.dimension};
        } else if (take('/')) {
            const auto right = unary();
            if (right.value == 0.0) throw std::runtime_error("Divisione per zero");
            if (right.dimension != Dimension::Scalar && right.dimension != left.dimension)
                throw std::runtime_error("Unità incompatibili nella divisione");
            left = {left.value / right.value,
                    right.dimension == Dimension::Scalar ? left.dimension : Dimension::Scalar};
        } else break;
        if (!std::isfinite(left.value)) throw std::runtime_error("Risultato numerico non finito");
    }
    return left;
}
Quantity Parser::sum() {
    auto left = product();
    while (true) {
        const bool add = take('+');
        if (!add && !take('-')) break;
        const auto right = product();
        if (left.dimension != right.dimension) throw std::runtime_error("Unità incompatibili nella somma");
        left.value += add ? right.value : -right.value;
        if (!std::isfinite(left.value)) throw std::runtime_error("Risultato numerico non finito");
    }
    return left;
}
Quantity Parser::parse() {
    if (input_.empty() || input_.size() > 4096) throw std::runtime_error("Espressione vuota o troppo lunga");
    auto value = sum();
    spaces();
    if (pos_ != input_.size()) throw std::runtime_error("Testo inatteso nell'espressione");
    return value;
}
}

std::string default_unit(Dimension dimension) {
    if (dimension == Dimension::Length) return "mm";
    if (dimension == Dimension::Angle) return "rad";
    return {};
}

std::vector<EvaluatedParameter> evaluate_parameters(const Document& document) {
    Resolver resolver(document);
    std::vector<EvaluatedParameter> results;
    results.reserve(document.parameters.size());
    for (const auto& parameter : document.parameters) {
        EvaluatedParameter result{parameter.name, parameter.expression, parameter.unit, std::nullopt, {}};
        try {
            const auto quantity = resolver.resolve(parameter.name);
            if (result.unit.empty()) result.unit = default_unit(quantity.dimension);
            const auto unit = units.find(result.unit);
            result.value = unit == units.end() ? quantity.value : quantity.value / unit->second.scale;
        } catch (const std::exception& e) { result.error = e.what(); }
        results.push_back(std::move(result));
    }
    return results;
}

bool evaluate_expression(const Document& document, std::string_view expression,
                         Quantity& result, std::string& error) {
    try {
        result = Resolver(document).expression(expression);
        error.clear();
        return true;
    } catch (const std::exception& e) {
        error = e.what();
        return false;
    }
}

} // namespace sagomacad::doc
