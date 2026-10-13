#include "automation/dispatch.hpp"

namespace sagomacad::automation {

nlohmann::json dispatch(engine::Engine& engine, std::string_view method,
                         const nlohmann::json& params) {
    try {
        if (method == "engine.commands") {
            if (!params.is_object() || !params.empty())
                return {{"error", {{"message", "engine.commands richiede parametri vuoti"}}}};
            return {{"result", engine.commands()}};
        }
        if (method == "engine.execute") {
            if (!params.is_object() || !params.contains("command") || !params["command"].is_string())
                return {{"error", {{"message", "engine.execute richiede command come stringa"}}}};
            for (auto it = params.begin(); it != params.end(); ++it)
                if (it.key() != "command" && it.key() != "params" && it.key() != "coalesce")
                    return {{"error", {{"message", "Parametro JSON sconosciuto: " + it.key()}}}};
            const auto arguments = params.value("params", nlohmann::json::object());
            std::string coalesce;
            if (params.contains("coalesce")) {
                if (!params["coalesce"].is_string())
                    return {{"error", {{"message", "coalesce deve essere una stringa"}}}};
                coalesce = params["coalesce"].get<std::string>();
            }
            const auto result = engine.execute(params["command"].get<std::string>(), arguments, coalesce);
            return result.ok ? nlohmann::json{{"result", result.value}}
                             : nlohmann::json{{"error", {{"message", result.error}}}};
        }
        return {{"error", {{"message", "Metodo sconosciuto: " + std::string(method)}}}};
    } catch (const std::exception& e) {
        return {{"error", {{"message", e.what()}}}};
    }
}

} // namespace sagomacad::automation
