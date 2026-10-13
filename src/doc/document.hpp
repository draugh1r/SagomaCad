#pragma once

#include <nlohmann/json.hpp>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace sagomacad::doc {

inline constexpr int format_version = 1;

enum class BodyKind { BRep, Mesh };
enum class FeatureKind { Exact, Sdf };

struct Parameter {
    std::string name;
    std::string expression;
    std::string unit;
    std::string comment;
    bool operator==(const Parameter&) const = default;
};

struct Feature {
    std::string id;
    std::string name;
    std::string type;
    FeatureKind kind = FeatureKind::Exact;
    bool suppressed = false;
    std::optional<std::string> error;
    nlohmann::json data = nlohmann::json::object();
    bool operator==(const Feature&) const = default;
};

struct Body {
    std::string id;
    std::string name;
    BodyKind kind = BodyKind::BRep;
    bool visible = true;
    std::string color = "#8B9BA6";
    bool operator==(const Body&) const = default;
};

struct Document {
    std::string name = "Nuovo documento";
    std::vector<Parameter> parameters;
    std::vector<std::string> timeline;
    std::size_t rollback = 0;
    std::map<std::string, Feature> features;
    std::vector<Body> bodies;
    bool operator==(const Document&) const = default;
};

nlohmann::json to_json(const Document& document);
bool from_json(const nlohmann::json& value, Document& document, std::string& error);
bool save_atomic(const Document& document, const std::filesystem::path& path, std::string& error);
bool load(const std::filesystem::path& path, Document& document, std::string& error);

} // namespace sagomacad::doc
