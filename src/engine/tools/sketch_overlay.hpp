#pragma once
#include "solver/sketch_solver.hpp"
namespace sagomacad::engine {
struct SketchXY { double x=0,y=0; };
struct SketchStroke {std::string id;std::vector<SketchXY> points;bool construction=false,fixed=false,reference=false;};
struct SketchLabel {std::string id,text,kind;SketchXY position;bool vertical=false;};
struct SketchOverlay {
    std::vector<SketchStroke> strokes;
    std::vector<std::vector<SketchXY>> profiles;
    std::vector<SketchLabel> dimensions,constraints;
};
SketchOverlay sketch_overlay(const solver::SketchResult&);
}
