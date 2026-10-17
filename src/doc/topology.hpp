#pragma once
#include <array>
#include <nlohmann/json.hpp>
#include <string>
namespace sagomacad::doc {
struct GeometrySignature {
    std::string kind,type;
    std::array<double,3> center{},direction{};
    double measure=0;
};
struct TopologyRef {
    std::string body,name;
    GeometrySignature signature;
};
inline void to_json(nlohmann::json& j,const GeometrySignature& s){j={{"kind",s.kind},{"type",s.type},{"center",s.center},{"direction",s.direction},{"measure",s.measure}};}
inline void from_json(const nlohmann::json& j,GeometrySignature& s){s.kind=j.at("kind");s.type=j.at("type");s.center=j.at("center");s.direction=j.at("direction");s.measure=j.at("measure");}
inline void to_json(nlohmann::json& j,const TopologyRef& r){j={{"body",r.body},{"name",r.name},{"signature",r.signature}};}
inline void from_json(const nlohmann::json& j,TopologyRef& r){r.body=j.at("body");r.name=j.at("name");if(j.contains("signature"))r.signature=j.at("signature").get<GeometrySignature>();}
}
