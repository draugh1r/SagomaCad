#include "doc/sketch.hpp"
#include <cmath>
#include <set>
#include <stdexcept>

namespace sagomacad::doc {
nlohmann::json sketch_json(const Sketch& s) {
    nlohmann::json j={{"plane",s.plane},{"points",nlohmann::json::array()},{"entities",nlohmann::json::array()},
        {"constraints",nlohmann::json::array()},{"dimensions",nlohmann::json::array()}};
    for(const auto& p:s.points) j["points"].push_back({{"id",p.id},{"x",p.x},{"y",p.y}});
    for(const auto& e:s.entities) j["entities"].push_back({{"id",e.id},{"kind",e.kind},{"points",e.points},{"radius",e.radius},{"construction",e.construction}});
    auto put=[&](const auto& cs,const char* key){for(const auto& c:cs)j[key].push_back({{"id",c.id},{"kind",c.kind},{"refs",c.refs},{"expression",c.expression},{"values",c.values}});};
    put(s.constraints,"constraints");put(s.dimensions,"dimensions");if(s.visible)j["visible"]=*s.visible;return j;
}
Sketch read_sketch(const nlohmann::json& j) {
    if(!j.is_object())throw std::runtime_error("sketch.invalid: dati dello schizzo non validi");
    Sketch s;s.plane=j.value("plane",std::string("XY"));
    if(j.contains("visible")&&!j["visible"].is_null())s.visible=j["visible"].get<bool>();
    if(s.plane!="XY"&&s.plane!="XZ"&&s.plane!="YZ")throw std::runtime_error("sketch.plane: piano non valido");
    for(const auto* key:{"points","entities","constraints","dimensions"})
        if(j.contains(key)&&(!j[key].is_array()||j[key].size()>256))throw std::runtime_error("sketch.limit: massimo 256 elementi per categoria");
    std::set<std::string> ids,point_ids,entity_ids;
    auto id=[&](const auto& v){auto x=v.at("id").template get<std::string>();if(x.empty()||x.size()>128||!ids.insert(x).second)throw std::runtime_error("sketch.id: ID duplicato o non valido");return x;};
    auto number=[](double x){if(!std::isfinite(x)||std::abs(x)>1e7)throw std::runtime_error("sketch.range: coordinata fuori intervallo");return x;};
    for(const auto& p:j.value("points",nlohmann::json::array())){SketchPoint v{id(p),number(p.value("x",0.0)),number(p.value("y",0.0))};point_ids.insert(v.id);s.points.push_back(v);}
    for(const auto& e:j.value("entities",nlohmann::json::array())){
        SketchEntity v{id(e),e.value("kind",std::string("point")),e.value("points",std::vector<std::string>{}),number(e.value("radius",1.0)),e.value("construction",false)};
        const size_t count=v.kind=="point"||v.kind=="circle"?1:v.kind=="line"?2:v.kind=="arc"?3:0;
        if(!count||v.points.size()!=count||v.radius<=0)throw std::runtime_error("sketch.entity: entità non valida");
        for(const auto& p:v.points)if(!point_ids.contains(p))throw std::runtime_error("sketch.reference: punto inesistente: "+p);
        entity_ids.insert(v.id);s.entities.push_back(v);
    }
    auto read=[&](auto& out,const char* key){for(const auto& c:j.value(key,nlohmann::json::array())){
        SketchConstraint v{id(c),c.at("kind").template get<std::string>(),c.value("refs",std::vector<std::string>{}),c.value("expression",std::string{}),c.value("values",std::vector<double>{})};
        if(v.refs.size()>4||v.values.size()>16||v.expression.size()>4096)throw std::runtime_error("sketch.limit: vincolo troppo grande");
        for(const auto& r:v.refs)if(!point_ids.contains(r)&&!entity_ids.contains(r))throw std::runtime_error("sketch.reference: riferimento inesistente: "+r);
        for(double x:v.values)number(x);out.push_back(v);
    }};read(s.constraints,"constraints");read(s.dimensions,"dimensions");return s;
}
}
