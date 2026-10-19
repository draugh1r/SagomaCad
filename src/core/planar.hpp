#pragma once
#include <array>
#include <set>
#include <string>
#include <vector>

namespace sagomacad::core::planar {
using Point=std::array<double,2>;
struct Curve {
    std::string id,first,last;
    bool line=false,closed=false;
    Point center{},a{},b{};
    double radius=0,begin=0,end=0;
};
Curve line(std::string id,Point a,Point b,std::string first,std::string last);
Curve circle(std::string id,Point center,double radius);
Curve arc(std::string id,Point center,double radius,Point a,Point b,std::string first,std::string last);
struct Span {std::size_t curve=0;double begin=0,end=0;std::string token;};
struct DirectedSpan {std::size_t span=0;bool forward=true;};
struct Arrangement {
    std::vector<Curve> curves;
    std::vector<Span> spans;
    std::vector<std::vector<DirectedSpan>> faces;
    std::set<std::string> split;
};
// Analytic intersections and a half-edge traversal. Tokens contain entity and
// intersection identities, never sampled coordinates or mesh indices.
Arrangement arrange(std::vector<Curve> curves);
Point evaluate(const Curve&,double parameter);
std::vector<Point> sample(const Arrangement&,const std::vector<DirectedSpan>&);
std::string selector(const Span&,bool forward);
}
