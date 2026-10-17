#pragma once
#include "doc/document.hpp"
#include "doc/topology.hpp"

namespace sagomacad::doc {
struct SketchPoint { std::string id; double x=0,y=0; };
struct SketchEntity {
    std::string id,kind; // point, line, circle, arc
    std::vector<std::string> points;
    double radius=1;
    bool construction=false;
    std::string reference_name; // projected, immutable topology; usable by constraints
};
struct SketchConstraint {
    std::string id,kind;
    std::vector<std::string> refs;
    std::string expression;
    std::vector<double> values; // fixed coordinates, captured when the constraint is made
};
struct Sketch {
    std::string plane="XY";
    std::optional<bool> visible; // unset: automatic visibility after a consuming feature
    std::optional<TopologyRef> support;
    std::array<double,3> origin{0,0,0},u{1,0,0},v{0,1,0};
    std::vector<SketchPoint> points;
    std::vector<SketchEntity> entities;
    std::vector<SketchConstraint> constraints,dimensions;
};
nlohmann::json sketch_json(const Sketch& sketch);
Sketch read_sketch(const nlohmann::json& json); // validates counts, ids, references and finite coordinates
}
