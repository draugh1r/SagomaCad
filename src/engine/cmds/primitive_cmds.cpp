#include "engine/command.hpp"
#include "engine/cmds/validation.hpp"
#include "doc/expression.hpp"
#include <algorithm>
#include <array>
#include <cmath>

namespace sagomacad::engine::cmds {
namespace {
double length_value(const doc::Document& document,const std::string& expression,bool positive,
                    const std::string& field) {
    doc::Quantity value;
    std::string error;
    if (!doc::evaluate_expression(document,expression,value,error))
        throw std::runtime_error(field+": "+error);
    if (value.dimension!=doc::Dimension::Length && value.dimension!=doc::Dimension::Scalar)
        throw std::runtime_error(field+": è richiesta una lunghezza");
    if (!std::isfinite(value.value) || std::abs(value.value)>1.0e9 || (positive && value.value<=0.0))
        throw std::runtime_error(field+": valore fuori intervallo");
    return value.value;
}
std::string expression_field(const nlohmann::json& object,const char* key) {
    return string_arg(object,key,true,4096);
}
}

std::vector<CommandSpec> primitive_specs() {
    return {{"primitive.add","Primitiva",{"Solido","Crea"},std::nullopt,
             "{kind: box|cylinder|sphere, dimensions: {width,depth,height|radius,height|radius}, position?: {x,y,z}, operation?: new|subtract, body?: id, name?: string}",{},
             [](Session& session,const nlohmann::json& params)->CommandResult {
                 only_keys(params,{"kind","dimensions","position","operation","body","name"});
                 const auto kind=string_arg(params,"kind",true,16);
                 if (kind!="box" && kind!="cylinder" && kind!="sphere")
                     return CommandResult::failure("Primitiva sconosciuta: "+kind);
                 const auto operation=params.contains("operation")?string_arg(params,"operation",true,16):"new";
                 if (operation!="new" && operation!="subtract")
                     return CommandResult::failure("operation deve essere new o subtract");
                 std::string target_body;
                 if (operation=="subtract") {
                     target_body=string_arg(params,"body",true,256);
                     const auto found=std::find_if(session.document.bodies.begin(),session.document.bodies.end(),
                         [&](const auto& body){return body.id==target_body && body.kind==doc::BodyKind::BRep;});
                     if (found==session.document.bodies.end()) return CommandResult::failure("Corpo B-rep da sottrarre non trovato: "+target_body);
                 } else if (params.contains("body")) return CommandResult::failure("body è ammesso solo per operation=subtract");
                 if (!params.contains("dimensions") || !params["dimensions"].is_object())
                     return CommandResult::failure("dimensions deve essere un oggetto");
                 const auto& dimensions=params["dimensions"];
                 if (kind=="box") only_keys(dimensions,{"width","depth","height"});
                 else if (kind=="cylinder") only_keys(dimensions,{"radius","height"});
                 else only_keys(dimensions,{"radius"});
                 nlohmann::json stored_dimensions=nlohmann::json::object();
                 const std::array<const char*,3> box_fields={"width","depth","height"};
                 const std::array<const char*,2> cylinder_fields={"radius","height"};
                 const std::array<const char*,1> sphere_fields={"radius"};
                 auto capture=[&](const char* field) {
                     const auto expression=expression_field(dimensions,field);
                     length_value(session.document,expression,true,field);
                     stored_dimensions[field]=expression;
                 };
                 if (kind=="box") for (const auto* field:box_fields) capture(field);
                 else if (kind=="cylinder") for (const auto* field:cylinder_fields) capture(field);
                 else for (const auto* field:sphere_fields) capture(field);
                 nlohmann::json position={{"x","0 mm"},{"y","0 mm"},{"z","0 mm"}};
                 if (params.contains("position")) {
                     if (!params["position"].is_object()) return CommandResult::failure("position deve essere un oggetto");
                     only_keys(params["position"],{"x","y","z"});
                     for (const auto* axis:{"x","y","z"}) if (params["position"].contains(axis))
                         position[axis]=expression_field(params["position"],axis);
                 }
                 for (const auto* axis:{"x","y","z"})
                     length_value(session.document,position[axis].get<std::string>(),false,std::string("position.")+axis);
                 const std::string default_name=operation=="subtract"?
                     (kind=="cylinder"?"Taglio cilindrico":kind=="sphere"?"Taglio sferico":"Taglio box"):
                     kind=="box"?"Box":kind=="cylinder"?"Cilindro":"Sfera";
                 const auto name=params.contains("name")?string_arg(params,"name",true,256):default_name;
                 std::size_t serial=session.document.timeline.size()+1;
                 std::string feature_id,body_id;
                 do {
                     feature_id="primitive-"+std::to_string(serial);
                     body_id="body-"+std::to_string(serial++);
                 } while (session.document.features.contains(feature_id));
                 if (operation=="subtract") body_id=target_body;
                 doc::Feature feature;
                 feature.id=feature_id;
                 feature.name=name;
                 feature.type="primitive."+kind;
                 feature.kind=doc::FeatureKind::Exact;
                 feature.data={{"dimensions",stored_dimensions},{"position",position},{"body_id",body_id},{"operation",operation}};
                 session.document.features.emplace(feature_id,std::move(feature));
                 if (operation=="new") session.document.bodies.push_back({body_id,name,doc::BodyKind::BRep,true,"#8B9BA6"});
                 session.document.timeline.push_back(feature_id);
                 session.document.rollback=session.document.timeline.size();
                 return {true,{{"feature_id",feature_id},{"body_id",body_id}}, {}};
             },true},
            {"primitive.edit","Modifica primitiva",{"Solido","Modifica"},std::nullopt,
             "{feature_id: string, dimensions: object, name?: string}",{},
             [](Session& session,const nlohmann::json& params)->CommandResult {
                 only_keys(params,{"feature_id","dimensions","name"});
                 const auto id=string_arg(params,"feature_id",true,256);
                 auto found=session.document.features.find(id);
                 if (found==session.document.features.end() || found->second.type.rfind("primitive.",0)!=0)
                     return CommandResult::failure("Primitiva sconosciuta: "+id);
                 if (!params.contains("dimensions") || !params["dimensions"].is_object())
                     return CommandResult::failure("dimensions deve essere un oggetto");
                 const auto& values=params["dimensions"];
                 const auto kind=found->second.type.substr(10);
                 if (kind=="box") only_keys(values,{"width","depth","height"});
                 else if (kind=="cylinder") only_keys(values,{"radius","height"});
                 else only_keys(values,{"radius"});
                 auto dimensions=found->second.data.at("dimensions");
                 for (const auto* field:{"width","depth","height","radius"}) if (values.contains(field)) {
                     const auto expression=expression_field(values,field);
                     length_value(session.document,expression,true,field);
                     dimensions[field]=expression;
                 }
                 found->second.data["dimensions"]=std::move(dimensions);
                 if (params.contains("name")) {
                     found->second.name=string_arg(params,"name",true,256);
                     const auto body_id=found->second.data.at("body_id").get<std::string>();
                     if (found->second.data.value("operation",std::string("new"))=="new")
                         for (auto& body:session.document.bodies) if (body.id==body_id) body.name=found->second.name;
                 }
                 return {true,{{"feature_id",id}}, {}};
             },true}};
}
} // namespace sagomacad::engine::cmds
