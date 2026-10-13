#include "engine/command.hpp"
#include "engine/cmds/validation.hpp"
#include "doc/expression.hpp"
#include <algorithm>

namespace sagomacad::engine {

nlohmann::json evaluated_parameters_json(const doc::Document& document) {
    nlohmann::json rows = nlohmann::json::array();
    for (const auto& parameter : doc::evaluate_parameters(document))
        rows.push_back({{"name", parameter.name}, {"expression", parameter.expression},
                        {"unit", parameter.unit},
                        {"value", parameter.value ? nlohmann::json(*parameter.value) : nlohmann::json(nullptr)},
                        {"error", parameter.error}});
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
}

std::vector<CommandSpec> parameter_specs() {
    return {
        {"param.set", "Imposta parametro", {"Solido", "Modifica"}, std::nullopt,
         "{name: string, expression: string, unit?: mm|cm|m|in|ft|deg|rad}", {},
         [](Session& session, const nlohmann::json& params) -> CommandResult {
             only_keys(params, {"name", "expression", "unit"});
             const auto name = string_arg(params, "name", true, 64);
             const auto expression = string_arg(params, "expression", true, 4096);
             if (!valid_name(name) || reserved_unit(name))
                 return CommandResult::failure("Nome di parametro non valido: " + name);
             auto candidate = session.document;
             auto it = std::find_if(candidate.parameters.begin(), candidate.parameters.end(),
                                    [&](const doc::Parameter& row) { return row.name == name; });
             if (it == candidate.parameters.end()) {
                 candidate.parameters.push_back({name, expression, {}});
                 it = std::prev(candidate.parameters.end());
             } else it->expression = expression;
             if (params.contains("unit")) it->unit = string_arg(params, "unit", false, 8);
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
         "{name: string}; fallisce se un altro parametro lo usa", {},
         [](Session& session, const nlohmann::json& params) -> CommandResult {
             only_keys(params, {"name"});
             const auto name = string_arg(params, "name", true, 64);
             auto candidate = session.document;
             const auto it = std::find_if(candidate.parameters.begin(), candidate.parameters.end(),
                                          [&](const doc::Parameter& row) { return row.name == name; });
             if (it == candidate.parameters.end()) return CommandResult::failure("Parametro sconosciuto: " + name);
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
