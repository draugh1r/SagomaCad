#include "ui/parameter_editor.hpp"
#include <algorithm>
#include <cctype>

namespace sagomacad::ui {
namespace {
std::string trim(std::string value) {
    const auto first=std::find_if_not(value.begin(),value.end(),[](unsigned char c){return std::isspace(c);});
    const auto last=std::find_if_not(value.rbegin(),value.rend(),[](unsigned char c){return std::isspace(c);}).base();
    return first>=last?std::string{}:std::string(first,last);
}
}
ParameterInput parse_parameter_input(std::string name, std::string expression) {
    name=trim(std::move(name));
    expression=trim(std::move(expression));
    const auto equal=expression.find('=');
    if (equal!=std::string::npos) {
        const auto left=trim(expression.substr(0,equal));
        const auto right=trim(expression.substr(equal+1));
        if (left.empty() || right.empty()) return {name,expression,"Usa la forma nome = espressione"};
        if (!name.empty() && name!=left)
            return {name,expression,"Il nome prima di '=' non coincide con il campo Nome"};
        name=left;
        expression=right;
    }
    return {std::move(name),std::move(expression),{}};
}
bool submit_parameter(State& state, engine::Engine& engine) {
    const auto parsed=parse_parameter_input(state.parameter_name.data(),state.parameter_expression.data());
    if (!parsed.error.empty()) { state.parameter_error=parsed.error; return false; }
    nlohmann::json params={{"name",parsed.name},{"expression",parsed.expression},
                           {"unit",std::string(state.parameter_unit.data())},
                           {"comment",std::string(state.parameter_comment.data())}};
    if (!state.editing_parameter_name.empty()) params["old_name"]=state.editing_parameter_name;
    const auto result=engine.execute("param.set",params);
    state.parameter_error=result.ok?std::string{}:result.error;
    if (result.ok) {
        state.parameter_editing=false;
        state.editing_parameter_name.clear();
    }
    return result.ok;
}
doc::EvaluatedParameter preview_parameter(const State& state, const engine::Engine& engine) {
    const auto parsed=parse_parameter_input(state.parameter_name.data(),state.parameter_expression.data());
    doc::EvaluatedParameter result{parsed.name,parsed.expression,state.parameter_unit.data(),std::nullopt,parsed.error};
    if (!parsed.error.empty()) return result;
    if (parsed.name.empty() && parsed.expression.empty()) return result;
    auto candidate=engine.session().document;
    auto row=std::find_if(candidate.parameters.begin(),candidate.parameters.end(),
        [&](const doc::Parameter& parameter) { return parameter.name==state.editing_parameter_name; });
    if (row==candidate.parameters.end()) {
        if (std::any_of(candidate.parameters.begin(),candidate.parameters.end(),
                        [&](const doc::Parameter& parameter){return parameter.name==parsed.name;})) {
            result.error="Nome già usato: "+parsed.name;
            return result;
        }
        candidate.parameters.push_back({parsed.name,parsed.expression,state.parameter_unit.data(),state.parameter_comment.data()});
    }
    else { row->name=parsed.name; row->expression=parsed.expression; row->unit=state.parameter_unit.data(); }
    for (const auto& evaluated:doc::evaluate_parameters(candidate))
        if (evaluated.name==parsed.name) return evaluated;
    result.error="Parametro non valido";
    return result;
}
} // namespace sagomacad::ui
