#pragma once
#include "doc/document.hpp"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace sagomacad::engine {

struct Status {
    bool ok = true;
    std::string error;
    static Status failure(std::string message) { return {false, std::move(message)}; }
};
struct CommandResult {
    bool ok = true;
    nlohmann::json value = nlohmann::json::object();
    std::string error;
    static CommandResult failure(std::string message) { return {false, nullptr, std::move(message)}; }
};

class History {
public:
    void clear();
    void record(const doc::Document& before, const doc::Document& after, std::string_view coalesce_key);
    bool can_undo() const { return cursor_ > 0; }
    bool can_redo() const { return cursor_ < entries_.size(); }
    bool undo(doc::Document& document);
    bool redo(doc::Document& document);
    std::size_t size() const { return entries_.size(); }
    std::size_t cursor() const { return cursor_; }
private:
    struct Entry { doc::Document before, after; std::string coalesce_key; };
    std::vector<Entry> entries_;
    std::size_t cursor_ = 0;
};

struct Session {
    doc::Document document;
    std::filesystem::path path;
    History history;
    std::string active_sketch; // editing state, not document geometry
};

struct CommandSpec {
    std::string id;
    std::string label;
    std::vector<std::string> menu;
    std::optional<std::string> shortcut;
    std::string params_doc;
    std::function<Status(const Session&)> enabled;
    std::function<CommandResult(Session&, const nlohmann::json&)> run;
    bool journal = false;
};

class Registry {
public:
    Registry();
    const CommandSpec* find(std::string_view id) const;
    const std::vector<CommandSpec>& specs() const { return specs_; }
private:
    std::vector<CommandSpec> specs_;
};

class Engine {
public:
    Session& session() { return session_; }
    const Session& session() const { return session_; }
    const Registry& registry() const { return registry_; }
    CommandResult execute(std::string_view id, const nlohmann::json& params = nlohmann::json::object(),
                          std::string_view coalesce_key = {});
    CommandResult execute_batch(const nlohmann::json& steps, bool stop_on_error = true);
    nlohmann::json commands() const;
private:
    Session session_;
    Registry registry_;
};

nlohmann::json evaluated_parameters_json(const doc::Document& document);

namespace cmds {
std::vector<CommandSpec> document_specs();
std::vector<CommandSpec> parameter_specs();
std::vector<CommandSpec> edit_specs();
std::vector<CommandSpec> primitive_specs();
std::vector<CommandSpec> export_specs();
std::vector<CommandSpec> body_specs();
std::vector<CommandSpec> feature_specs();
std::vector<CommandSpec> measure_specs();
std::vector<CommandSpec> sketch_specs();
std::vector<CommandSpec> model_specs();
}

} // namespace sagomacad::engine
