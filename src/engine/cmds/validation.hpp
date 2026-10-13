#pragma once
#include <nlohmann/json.hpp>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <string_view>

namespace sagomacad::engine::cmds {

inline void only_keys(const nlohmann::json& params, std::initializer_list<std::string_view> allowed) {
    for (auto it = params.begin(); it != params.end(); ++it) {
        bool known = false;
        for (const auto key : allowed) if (it.key() == key) { known = true; break; }
        if (!known) throw std::runtime_error("Parametro JSON sconosciuto: " + it.key());
    }
}
inline std::string string_arg(const nlohmann::json& params, const char* key,
                              bool required = true, std::size_t maximum = 4096) {
    const auto it = params.find(key);
    if (it == params.end()) {
        if (required) throw std::runtime_error(std::string("Manca il parametro ") + key);
        return {};
    }
    if (!it->is_string()) throw std::runtime_error(std::string(key) + " deve essere una stringa");
    std::string value = it->get<std::string>();
    if ((required && value.empty()) || value.size() > maximum)
        throw std::runtime_error(std::string(key) + " vuoto o troppo lungo");
    return value;
}
inline bool valid_name(std::string_view name) {
    if (name.empty() || name.size() > 64) return false;
    auto head = [](unsigned char c) {
        return c == '_' || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c >= 128;
    };
    if (!head(static_cast<unsigned char>(name.front()))) return false;
    for (unsigned char c : name.substr(1)) if (!head(c) && !(c >= '0' && c <= '9')) return false;
    return true;
}

} // namespace sagomacad::engine::cmds
