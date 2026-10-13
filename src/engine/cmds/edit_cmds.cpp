#include "engine/command.hpp"
#include "engine/cmds/validation.hpp"

namespace sagomacad::engine::cmds {

std::vector<CommandSpec> edit_specs() {
    return {
        {"edit.undo", "Annulla", {"Modifica"}, std::string("Ctrl+Z"),
         "{}; annulla l'ultima modifica",
         [](const Session& session) -> Status {
             return session.history.can_undo() ? Status{} : Status::failure("Nessuna modifica da annullare");
         },
         [](Session& session, const nlohmann::json& params) -> CommandResult {
             only_keys(params, {});
             if (!session.history.undo(session.document)) return CommandResult::failure("Nessuna modifica da annullare");
             return {true, {{"undo", true}}, {}};
         }, false},
        {"edit.redo", "Ripeti", {"Modifica"}, std::string("Ctrl+Y"),
         "{}; ripristina l'ultima modifica annullata",
         [](const Session& session) -> Status {
             return session.history.can_redo() ? Status{} : Status::failure("Nessuna modifica da ripetere");
         },
         [](Session& session, const nlohmann::json& params) -> CommandResult {
             only_keys(params, {});
             if (!session.history.redo(session.document)) return CommandResult::failure("Nessuna modifica da ripetere");
             return {true, {{"redo", true}}, {}};
         }, false},
    };
}

} // namespace sagomacad::engine::cmds
