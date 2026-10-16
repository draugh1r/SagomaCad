#pragma once
#include "doc/sketch.hpp"
#include <array>
namespace sagomacad::solver {
using ProfilePoint=std::array<double,2>;
struct ProfileRegion {
    std::string id;
    std::vector<std::string> boundary;
    std::vector<std::vector<std::string>> holes;
    std::vector<ProfilePoint> polygon;
    std::vector<std::vector<ProfilePoint>> hole_polygons;
};
std::vector<ProfilePoint> sample_loop(const doc::Sketch&,const std::vector<std::string>&);
bool contains(const std::vector<ProfilePoint>&,double x,double y);
bool contains(const ProfileRegion&,double x,double y);
std::vector<ProfileRegion> profile_regions(const doc::Sketch&,const std::vector<std::vector<std::string>>&);
nlohmann::json regions_json(const std::vector<ProfileRegion>&);
}
