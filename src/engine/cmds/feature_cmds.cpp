#include "engine/command.hpp"
#include "engine/cmds/validation.hpp"
#include "doc/expression.hpp"
#include <algorithm>
#include <cmath>

namespace sagomacad::engine::cmds {
CommandResult edit_model_feature(Session&,const nlohmann::json&,const std::string&);
CommandResult edit_surface_feature(Session&,const nlohmann::json&,const std::string&);
CommandResult edit_hole_feature(Session&,const nlohmann::json&,const std::string&);
namespace {
std::string feature_id(const nlohmann::json& params) { return string_arg(params,params.contains("feature_id")?"feature_id":"id",true,256); }
double checked_length(const doc::Document& document,const std::string& expression,const std::string& field) {
    doc::Quantity value; std::string error;
    if (!doc::evaluate_expression(document,expression,value,error)) throw std::runtime_error(field+": "+error);
    if (value.dimension!=doc::Dimension::Length && value.dimension!=doc::Dimension::Scalar)
        throw std::runtime_error(field+": è richiesta una lunghezza");
    if (!std::isfinite(value.value) || value.value<=0.0 || value.value>1.0e9)
        throw std::runtime_error(field+": valore fuori intervallo");
    return value.value;
}
}
std::vector<CommandSpec> feature_specs() {
    return {
        {"feature.edit","Modifica feature",{"Timeline","Modifica"},std::nullopt,
         "{feature_id: id, dimensions?: primitive dimensions, params?: extrude/revolve parameter changes, name?: string}",{},
         [](Session& session,const nlohmann::json& params)->CommandResult {
             const auto id=feature_id(params);
             const auto found=session.document.features.find(id);
             if(found!=session.document.features.end()&&found->second.type=="feature.hole")return edit_hole_feature(session,params,id);
             if(found!=session.document.features.end()&&(found->second.type=="feature.fillet"||found->second.type=="feature.chamfer"))return edit_surface_feature(session,params,id);
             if(found!=session.document.features.end()&&(found->second.type=="feature.extrude"||found->second.type=="feature.revolve"))return edit_model_feature(session,params,id);
             only_keys(params,{"feature_id","id","dimensions","name"});
             if (found==session.document.features.end() || found->second.type.rfind("primitive.",0)!=0)
                 return CommandResult::failure("Feature modificabile non trovata: "+id);
             if (!params.contains("dimensions") || !params["dimensions"].is_object())
                 return CommandResult::failure("dimensions deve essere un oggetto");
             const auto& changes=params["dimensions"];
             const auto type=found->second.type.substr(10);
             if (type=="box") only_keys(changes,{"width","depth","height"});
             else if (type=="cylinder") only_keys(changes,{"radius","height"});
             else if (type=="sphere") only_keys(changes,{"radius"});
             else return CommandResult::failure("Tipo di feature non modificabile");
             auto dimensions=found->second.data.at("dimensions");
             for (auto it=changes.begin();it!=changes.end();++it) {
                 if (!it.value().is_string()) return CommandResult::failure(it.key()+" deve essere un'espressione");
                 const auto expression=it.value().get<std::string>();
                 if (expression.empty() || expression.size()>4096) return CommandResult::failure(it.key()+": espressione vuota o troppo lunga");
                 checked_length(session.document,expression,it.key());
                 dimensions[it.key()]=expression;
             }
             std::string name=found->second.name;
             if (params.contains("name")) name=string_arg(params,"name",true,256);
             if (!found->second.data.contains("body_id") || !found->second.data["body_id"].is_string())
                 return CommandResult::failure("Corpo della feature non valido");
             const auto body_id=found->second.data["body_id"].get<std::string>();
             found->second.data["dimensions"]=std::move(dimensions);
             found->second.name=name;
             if (found->second.data.value("operation",std::string("new"))=="new")
                 for (auto& body:session.document.bodies) if (body.id==body_id) body.name=name;
             return {true,{{"feature_id",id}}, {}};
         },true},
        {"feature.suppress","Sopprimi feature",{"Timeline","Modifica"},std::nullopt,
         "{feature_id: string, suppressed: bool}",{},
         [](Session& session,const nlohmann::json& params)->CommandResult {
             only_keys(params,{"feature_id","id","suppressed"});
             const auto id=feature_id(params);
             if (!params.contains("suppressed") || !params["suppressed"].is_boolean())
                 return CommandResult::failure("suppressed deve essere un booleano");
             const auto found=session.document.features.find(id);
             if (found==session.document.features.end()) return CommandResult::failure("Feature sconosciuta: "+id);
             found->second.suppressed=params["suppressed"].get<bool>();
             return {true,{{"feature_id",id},{"suppressed",found->second.suppressed}}, {}};
         },true},
        {"feature.delete","Elimina feature",{"Timeline","Modifica"},std::nullopt,
         "{feature_id: string}",{},
         [](Session& session,const nlohmann::json& params)->CommandResult {
             only_keys(params,{"feature_id","id"});
             const auto id=feature_id(params);
             const auto found=session.document.features.find(id);
             if (found==session.document.features.end()) return CommandResult::failure("Feature sconosciuta: "+id);
             std::string body_id;
             if (found->second.data.is_object() && found->second.data.contains("body_id") && found->second.data["body_id"].is_string())
                 body_id=found->second.data["body_id"].get<std::string>();
             session.document.features.erase(found);
             auto& timeline=session.document.timeline;
             const auto at=std::find(timeline.begin(),timeline.end(),id);
             if(at!=timeline.end()&&std::size_t(at-timeline.begin())<session.document.rollback)--session.document.rollback;
             timeline.erase(std::remove(timeline.begin(),timeline.end(),id),timeline.end());
             session.document.rollback=std::min(session.document.rollback,timeline.size());
             if(session.active_sketch==id)session.active_sketch.clear();
             bool referenced=false;
             for (const auto& [other_id,feature]:session.document.features) {
                 if (feature.data.is_object() && feature.data.contains("body_id") && feature.data["body_id"].is_string() &&
                     feature.data["body_id"].get<std::string>()==body_id) referenced=true;
                 if(feature.data.contains("target_bodies")&&feature.data["target_bodies"].is_array())for(const auto& target:feature.data["target_bodies"])if(target.is_string()&&target.get<std::string>()==body_id)referenced=true;
             }
             if (!body_id.empty() && !referenced) {
                 auto& bodies=session.document.bodies;
                 bodies.erase(std::remove_if(bodies.begin(),bodies.end(),[&](const auto& b){return b.id==body_id;}),bodies.end());
             }
             return {true,{{"feature_id",id}}, {}};
         },true}
    };
}
}
