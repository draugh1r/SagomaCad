#include "engine/command.hpp"
#include "engine/cmds/validation.hpp"
#include "regen/regenerator.hpp"

namespace sagomacad::engine::cmds {
namespace {
std::filesystem::path from_utf8(const std::string& value) { return std::filesystem::u8path(value); }
std::string to_utf8(const std::filesystem::path& path) {
    const auto bytes = path.u8string();
    return {bytes.begin(), bytes.end()};
}
CommandResult save(Session& session, const nlohmann::json& params, bool require_path) {
    only_keys(params, {"path"});
    std::filesystem::path path = session.path;
    if (params.contains("path")) path = from_utf8(string_arg(params, "path"));
    else if (require_path) return CommandResult::failure("Salva con nome richiede un percorso");
    if (path.empty()) return CommandResult::failure("Nessun percorso: usare Salva con nome");
    std::string error;
    if (!doc::save_atomic(session.document, path, error)) return CommandResult::failure(error);
    session.path = std::filesystem::absolute(path);
    return {true, {{"path", to_utf8(session.path)}}, {}};
}
}

std::vector<CommandSpec> document_specs() {
    return {
        {"document.new", "Nuovo documento", {"File"}, std::string("Ctrl+N"),
         "{name?: string}; crea un documento vuoto", {},
         [](Session& session, const nlohmann::json& params) -> CommandResult {
             only_keys(params, {"name"});
             doc::Document fresh;
             if (params.contains("name")) fresh.name = string_arg(params, "name", true, 256);
             session.document = std::move(fresh);
             session.path.clear();
             session.history.clear();
             return {true, {{"name", session.document.name}}, {}};
         }, false},
        {"document.open", "Apri documento", {"File"}, std::string("Ctrl+O"),
         "{path: string}; apre un file .sagomacad", {},
         [](Session& session, const nlohmann::json& params) -> CommandResult {
             only_keys(params, {"path"});
             const auto path = from_utf8(string_arg(params, "path"));
             doc::Document loaded;
             std::string error;
             if (!doc::load(path, loaded, error)) return CommandResult::failure(error);
             session.document = std::move(loaded);
             session.path = std::filesystem::absolute(path);
             session.history.clear();
             return {true, {{"path", to_utf8(session.path)}}, {}};
         }, false},
        {"document.save", "Salva", {"File"}, std::string("Ctrl+S"),
         "{path?: string}; salva nel percorso corrente o specificato", {},
         [](Session& session, const nlohmann::json& params) { return save(session, params, false); }, false},
        {"document.saveAs", "Salva con nome", {"File"}, std::string("Ctrl+Shift+S"),
         "{path: string}; salva in un nuovo percorso", {},
         [](Session& session, const nlohmann::json& params) { return save(session, params, true); }, false},
        {"document.inspect", "Esamina documento", {"File"}, std::nullopt,
         "{}; restituisce modello, parametri calcolati e stato della storia", {},
         [](Session& session, const nlohmann::json& params) -> CommandResult {
             only_keys(params, {});
             regen::Regenerator regenerator;
             const auto& snapshot=regenerator.evaluate(session.document);
             nlohmann::json feature_states=nlohmann::json::array(),body_states=nlohmann::json::array();
             for (const auto& feature:snapshot.features)
                 feature_states.push_back({{"id",feature.id},{"error",feature.error}});
             for (const auto& body:snapshot.bodies)
                 body_states.push_back({{"id",body.id},{"error",body.error},{"has_shape",body.final.has_value()}});
             return {true, {{"document", doc::to_json(session.document)},
                            {"parameters", evaluated_parameters_json(session.document)},
                            {"regeneration",{{"features",feature_states},{"bodies",body_states}}},
                            {"path", to_utf8(session.path)},
                            {"history", {{"undo", session.history.can_undo()},
                                         {"redo", session.history.can_redo()},
                                         {"steps", session.history.size()}}}}, {}};
         }, false},
    };
}

} // namespace sagomacad::engine::cmds
