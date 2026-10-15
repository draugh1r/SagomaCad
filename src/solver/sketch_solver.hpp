#pragma once
#include "doc/sketch.hpp"
#include <map>

namespace sagomacad::solver {
struct SketchResult {
    doc::Sketch sketch;
    std::string state="free",error;
    int degrees_of_freedom=0;
    double residual=0;
    std::vector<std::string> redundant,conflicting;
    std::map<std::string,bool> fixed_points;
    std::vector<std::vector<std::string>> profiles;
};
SketchResult solve(const doc::Document&,const doc::Sketch&);
nlohmann::json inspect(const SketchResult&);
// Adds a temporary positional target, then removes it from the returned sketch.
SketchResult drag(const doc::Document&,const doc::Sketch&,const std::string& point,double x,double y);
}
