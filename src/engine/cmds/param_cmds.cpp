#include "engine/command.hpp"
#include "engine/cmds/validation.hpp"
#include "doc/expression.hpp"
#include <algorithm>
#include <cctype>

namespace sagomacad::engine {

nlohmann::json evaluated_parameters_json(const doc::Document& document) {
    nlohmann::json rows = nlohmann::json::array();
    const auto evaluated=doc::evaluate_parameters(document);
    for (std::size_t i=0;i<evaluated.size();++i) {
        const auto& parameter=evaluated[i];
        rows.push_back({{"name", parameter.name}, {"expression", parameter.expression},
                        {"unit", parameter.unit}, {"comment", document.parameters[i].comment},
                        {"value", parameter.value ? nlohmann::json(*parameter.value) : nlohmann::json(nullptr)},
                        {"error", parameter.error}});
    }
    return rows;
}

namespace cmds {
namespace {
CommandResult invalid_parameter(const doc::Document& document) {
    for (const auto& row : doc::evaluate_parameters(document))
        if (!row.error.empty()) return CommandResult::failure(row.name + ": " + row.error);
    return {};
}
bool reserved_unit(std::string_view name) {
    return name == "mm" || name == "cm" || name == "m" || name == "in" ||
           name == "ft" || name == "deg" || name == "rad";
}
bool identifier_byte(unsigned char c) {
    return c=='_' || std::isalnum(c) || c>=128;
}
void rename_reference(std::string& expression, std::string_view before, std::string_view after) {
    std::string rewritten;
    rewritten.reserve(expression.size()+after.size());
    for (std::size_t i=0;i<expression.size();) {
        const bool match=expression.compare(i,before.size(),before)==0 &&
            (i==0 || !identifier_byte(static_cast<unsigned char>(expression[i-1]))) &&
            (i+before.size()==expression.size() ||
             !identifier_byte(static_cast<unsigned char>(expression[i+before.size()])));
        if (match) { rewritten.append(after); i+=before.size(); }
        else rewritten.push_back(expression[i++]);
    }
    expression=std::move(rewritten);
}
template<class Visit>
void feature_expressions(doc::Document& document,Visit visit) {
    for (auto& [id,feature]:document.features) {
        if (feature.type.rfind("primitive.",0)!=0 || !feature.data.is_object()) continue;
        for (const char* key:{"dimensions","position"}) {
            auto found=feature.data.find(key);
            if (found==feature.data.end() || !found->is_object()) continue;
            for (auto& value:*found) if (value.is_string()) {
                auto expression=value.get<std::string>();
                visit(expression,feature);
                value=std::move(expression);
            }
        }
    }
}
}

std::vector<CommandSpec> parameter_specs() {
    return {
        {"param.set", "Imposta parametro", {"Solido", "Modifica"}, std::nullopt,
         "{name: string, expression: string, unit?: mm|cm|m|in|ft|deg|rad, comment?: string, old_name?: string}", {},
         [](Session& session, const nlohmann::json& params) -> CommandResult {
             only_keys(params, {"name", "expression", "unit", "comment", "old_name"});
             const auto name = string_arg(params, "name", true, 64);
             const auto expression = string_arg(params, "expression", true, 4096);
             const auto old_name = params.contains("old_name") ? string_arg(params,"old_name",true,64) : name;
             if (!valid_name(name) || reserved_unit(name))
                 return CommandResult::failure("Nome di parametro non valido: " + name);
             auto candidate = session.document;
             auto it = std::find_if(candidate.parameters.begin(), candidate.parameters.end(),
                                    [&](const doc::Parameter& row) { return row.name == old_name; });
             if (it == candidate.parameters.end()) {
                 if (old_name != name) return CommandResult::failure("Parametro da rinominare non trovato: " + old_name);
                 candidate.parameters.push_back({name, expression, {}, {}});
                 it = std::prev(candidate.parameters.end());
             } else {
                 if (old_name != name) {
                     const auto duplicate=std::find_if(candidate.parameters.begin(),candidate.parameters.end(),
                         [&](const doc::Parameter& row) { return row.name==name; });
                     if (duplicate!=candidate.parameters.end()) return CommandResult::failure("Nome già usato: " + name);
                     for (auto& row:candidate.parameters) rename_reference(row.expression,old_name,name);
                     feature_expressions(candidate,[&](std::string& value,const doc::Feature&) {
                         rename_reference(value,old_name,name);
                     });
                     it->name=name;
                 }
                 it->expression=expression;
                 if (old_name!=name) rename_reference(it->expression,old_name,name);
             }
             if (params.contains("unit")) it->unit = string_arg(params, "unit", false, 8);
             if (params.contains("comment")) it->comment = string_arg(params,"comment",false,512);
             const auto error = invalid_parameter(candidate);
             if (!error.ok) return error;
             session.document = std::move(candidate);
             for (const auto& row : evaluated_parameters_json(session.document))
                 if (row.at("name") == name) return {true, row, {}};
             return CommandResult::failure("Parametro non trovato dopo la modifica");
         }, true},
        {"param.list", "Cambia parametri", {"Solido", "Modifica"}, std::nullopt,
         "{}; restituisce nome, espressione, valore calcolato e unità", {},
         [](Session& session, const nlohmann::json& params) -> CommandResult {
             only_keys(params, {});
             return {true, evaluated_parameters_json(session.document), {}};
         }, false},
        {"param.delete", "Elimina parametro", {"Solido", "Modifica"}, std::nullopt,
         "{name: string}; fallisce se un parametro o una feature lo usa", {},
         [](Session& session, const nlohmann::json& params) -> CommandResult {
             only_keys(params, {"name"});
             const auto name = string_arg(params, "name", true, 64);
             auto candidate = session.document;
             const auto it = std::find_if(candidate.parameters.begin(), candidate.parameters.end(),
                                          [&](const doc::Parameter& row) { return row.name == name; });
             if (it == candidate.parameters.end()) return CommandResult::failure("Parametro sconosciuto: " + name);
             std::string referenced_by;
             feature_expressions(candidate,[&](std::string& value,const doc::Feature& feature) {
                 const auto original=value;
                 rename_reference(value,name,{});
                 if (value!=original) referenced_by=feature.name.empty()?feature.id:feature.name;
             });
             if (!referenced_by.empty())
                 return CommandResult::failure("Il parametro "+name+" è usato dalla feature "+referenced_by);
             candidate.parameters.erase(it);
             const auto error = invalid_parameter(candidate);
             if (!error.ok) return error;
             session.document = std::move(candidate);
             return {true, {{"deleted", name}}, {}};
         }, true},
    };
}

} // namespace cmds
} // namespace sagomacad::engine
