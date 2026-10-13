#include "engine/command.hpp"
#include <algorithm>
#include <stdexcept>

namespace sagomacad::engine {

void History::clear() { entries_.clear(); cursor_ = 0; }

void History::record(const doc::Document& before, const doc::Document& after, std::string_view coalesce_key) {
    if (before == after) return;
    const bool had_redo = cursor_ < entries_.size();
    entries_.erase(entries_.begin() + static_cast<std::ptrdiff_t>(cursor_), entries_.end());
    if (!had_redo && !coalesce_key.empty() && cursor_ > 0 && entries_.back().coalesce_key == coalesce_key) {
        entries_.back().after = after;
        return;
    }
    entries_.push_back({before, after, std::string(coalesce_key)});
    cursor_ = entries_.size();
}

bool History::undo(doc::Document& document) {
    if (!can_undo()) return false;
    document = entries_[--cursor_].before;
    return true;
}
bool History::redo(doc::Document& document) {
    if (!can_redo()) return false;
    document = entries_[cursor_++].after;
    return true;
}

Registry::Registry() {
    for (auto group : {cmds::document_specs(), cmds::parameter_specs(), cmds::edit_specs(),
                       cmds::primitive_specs(), cmds::export_specs(), cmds::body_specs(), cmds::feature_specs(),
                       cmds::measure_specs()})
        for (auto& spec : group) {
            if (find(spec.id)) throw std::logic_error("ID comando duplicato: " + spec.id);
            specs_.push_back(std::move(spec));
        }
}
const CommandSpec* Registry::find(std::string_view id) const {
    const auto it = std::find_if(specs_.begin(), specs_.end(),
                                 [id](const CommandSpec& spec) { return spec.id == id; });
    return it == specs_.end() ? nullptr : &*it;
}

CommandResult Engine::execute(std::string_view id, const nlohmann::json& params, std::string_view coalesce_key) {
    try {
        if (id.empty() || id.size() > 128) return CommandResult::failure("ID comando non valido");
        const auto* spec = registry_.find(id);
        if (!spec) return CommandResult::failure("Comando sconosciuto: " + std::string(id));
        if (!params.is_object()) return CommandResult::failure("I parametri del comando devono essere un oggetto JSON");
        if (coalesce_key.size() > 128) return CommandResult::failure("Chiave di coalescenza troppo lunga");
        const Status enabled = spec->enabled ? spec->enabled(session_) : Status{};
        if (!enabled.ok) return CommandResult::failure(enabled.error);
        const doc::Document before = spec->journal ? session_.document : doc::Document{};
        CommandResult result = spec->run(session_, params);
        if (!result.ok) return result;
        if (spec->journal)
            session_.history.record(before, session_.document,
                                    coalesce_key.empty() ? std::string{} : spec->id + ":" + std::string(coalesce_key));
        return result;
    } catch (const std::exception& e) {
        return CommandResult::failure(std::string("Errore nel comando: ") + e.what());
    } catch (...) {
        return CommandResult::failure("Errore imprevisto nel comando");
    }
}

CommandResult Engine::execute_batch(const nlohmann::json& steps, bool stop_on_error) {
    try {
        if (!steps.is_array() || steps.empty() || steps.size()>64)
            return CommandResult::failure("steps deve contenere da 1 a 64 comandi");
        Engine staged;
        staged.session_=session_;
        nlohmann::json results=nlohmann::json::array();
        for (std::size_t i=0;i<steps.size();++i) {
            const auto& step=steps[i];
            if (!step.is_object() || !step.contains("command") || !step["command"].is_string())
                return CommandResult::failure("Passo "+std::to_string(i)+": command deve essere una stringa");
            for (auto it=step.begin();it!=step.end();++it)
                if (it.key()!="command" && it.key()!="params")
                    return CommandResult::failure("Passo "+std::to_string(i)+": parametro sconosciuto "+it.key());
            const auto id=step["command"].get<std::string>();
            const auto* spec=registry_.find(id);
            if (!spec || !spec->journal)
                return CommandResult::failure("Passo "+std::to_string(i)+": il batch accetta solo comandi modificabili registrati");
            const auto params=step.value("params",nlohmann::json::object());
            const auto before_step=staged.session_;
            const auto result=staged.execute(id,params);
            if (!result.ok) {
                staged.session_=before_step;
                if (stop_on_error) return CommandResult::failure("Passo "+std::to_string(i)+" ("+id+"): "+result.error);
                results.push_back({{"ok",false},{"command",id},{"error",result.error}});
            } else results.push_back({{"ok",true},{"command",id},{"value",result.value}});
        }
        const auto before=session_.document;
        session_.document=std::move(staged.session_.document);
        session_.history.record(before,session_.document,{});
        return {true,{{"results",std::move(results)},{"history_steps",session_.history.size()}},{}};
    } catch (const std::exception& e) { return CommandResult::failure(std::string("Batch non valido: ")+e.what()); }
      catch (...) { return CommandResult::failure("Errore imprevisto nel batch"); }
}

nlohmann::json Engine::commands() const {
    nlohmann::json result = nlohmann::json::array();
    for (const auto& spec : registry_.specs()) {
        Status available;
        try { available = spec.enabled ? spec.enabled(session_) : Status{}; }
        catch (const std::exception& e) { available = Status::failure(e.what()); }
        result.push_back({{"id", spec.id}, {"label", spec.label}, {"menu", spec.menu},
                          {"shortcut", spec.shortcut ? nlohmann::json(*spec.shortcut) : nlohmann::json(nullptr)},
                          {"params_doc", spec.params_doc}, {"enabled", available.ok},
                          {"reason", available.error}, {"journal", spec.journal}});
    }
    return result;
}

} // namespace sagomacad::engine
