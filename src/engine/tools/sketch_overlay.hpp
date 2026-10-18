#pragma once
#include "solver/sketch_solver.hpp"
namespace sagomacad::engine {
struct SketchXY { double x=0,y=0; };
struct SketchStroke {std::string id;std::vector<SketchXY> points;bool construction=false,fixed=false,reference=false;};
// Geometry-owned annotation contract, shared by live inputs and stored dimensions.
struct SketchMeasure {
    int field=0;
    std::string label,unit;
    SketchXY a,b,position,outward;
    std::vector<std::string> refs;
};
struct SketchLabel {std::string id,text,kind;SketchXY position;bool vertical=false;SketchMeasure measure;};
struct SketchOverlay {
    std::vector<SketchStroke> strokes;
    std::vector<std::vector<SketchXY>> profiles;
    std::vector<SketchLabel> dimensions,constraints;
};
SketchOverlay sketch_overlay(const solver::SketchResult&);
}
