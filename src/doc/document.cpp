#include "doc/document.hpp"
#include <atomic>
#include <chrono>
#include <fstream>
#include <limits>
#include <system_error>
#ifdef _WIN32
#include <windows.h>
#endif

namespace sagomacad::doc {
namespace {
std::string body_kind(BodyKind kind) { return kind == BodyKind::Mesh ? "mesh" : "brep"; }
std::string feature_kind(FeatureKind kind) { return kind == FeatureKind::Sdf ? "sdf" : "exact"; }

template<class T>
T field(const nlohmann::json& object, const char* key, T fallback) {
    const auto it = object.find(key);
    return it == object.end() || it->is_null() ? fallback : it->get<T>();
}

bool object_or_null(const nlohmann::json& value) { return value.is_object() || value.is_null(); }
}

nlohmann::json to_json(const Document& document) {
    nlohmann::json parameters = nlohmann::json::array();
    for (const auto& parameter : document.parameters)
        parameters.push_back({{"name", parameter.name}, {"expression", parameter.expression}, {"unit", parameter.unit}});

    nlohmann::json features = nlohmann::json::object();
    for (const auto& [key, feature] : document.features)
        features[key] = {{"id", feature.id}, {"name", feature.name}, {"type", feature.type},
                         {"kind", feature_kind(feature.kind)}, {"suppressed", feature.suppressed},
                         {"error", feature.error}, {"data", feature.data}};

    nlohmann::json bodies = nlohmann::json::array();
    for (const auto& body : document.bodies)
        bodies.push_back({{"id", body.id}, {"name", body.name}, {"kind", body_kind(body.kind)},
                          {"visible", body.visible}, {"color", body.color}});

    return {{"format_version", format_version}, {"name", document.name},
            {"parameters", parameters}, {"timeline", document.timeline},
            {"rollback", document.rollback}, {"features", features}, {"bodies", bodies}};
}

bool from_json(const nlohmann::json& value, Document& document, std::string& error) {
    try {
        if (!value.is_object()) { error = "Il documento deve essere un oggetto JSON"; return false; }
        const int version = field(value, "format_version", format_version);
        if (version < 1 || version > format_version) {
            error = "Versione del formato .sagomacad non supportata";
            return false;
        }
        Document result;
        result.name = field(value, "name", result.name);
        if (const auto it = value.find("parameters"); it != value.end() && !it->is_null()) {
            if (!it->is_array()) throw std::runtime_error("parameters deve essere un array");
            for (const auto& item : *it) {
                if (!item.is_object()) throw std::runtime_error("un parametro deve essere un oggetto");
                result.parameters.push_back({field(item, "name", std::string{}),
                                             field(item, "expression", std::string{}),
                                             field(item, "unit", std::string{})});
            }
        }
        if (const auto it = value.find("timeline"); it != value.end() && !it->is_null())
            result.timeline = it->get<std::vector<std::string>>();
        result.rollback = field(value, "rollback", std::size_t{0});
        if (result.rollback > result.timeline.size()) throw std::runtime_error("rollback oltre la timeline");
        if (const auto it = value.find("features"); it != value.end() && !it->is_null()) {
            if (!it->is_object()) throw std::runtime_error("features deve essere un oggetto");
            for (auto member = it->begin(); member != it->end(); ++member) {
                const auto& item = member.value();
                if (!item.is_object()) throw std::runtime_error("una feature deve essere un oggetto");
                Feature feature;
                feature.id = field(item, "id", member.key());
                feature.name = field(item, "name", std::string{});
                feature.type = field(item, "type", std::string{});
                const std::string kind = field(item, "kind", std::string{"exact"});
                if (kind == "exact") feature.kind = FeatureKind::Exact;
                else if (kind == "sdf") feature.kind = FeatureKind::Sdf;
                else throw std::runtime_error("kind della feature non valido: " + kind);
                feature.suppressed = field(item, "suppressed", false);
                if (const auto e = item.find("error"); e != item.end() && !e->is_null())
                    feature.error = e->get<std::string>();
                if (const auto d = item.find("data"); d != item.end() && !d->is_null()) {
                    if (!object_or_null(*d)) throw std::runtime_error("data della feature deve essere un oggetto");
                    feature.data = *d;
                }
                result.features.emplace(member.key(), std::move(feature));
            }
        }
        if (const auto it = value.find("bodies"); it != value.end() && !it->is_null()) {
            if (!it->is_array()) throw std::runtime_error("bodies deve essere un array");
            for (const auto& item : *it) {
                if (!item.is_object()) throw std::runtime_error("un corpo deve essere un oggetto");
                Body body;
                body.id = field(item, "id", std::string{});
                body.name = field(item, "name", std::string{});
                const std::string kind = field(item, "kind", std::string{"brep"});
                if (kind == "brep") body.kind = BodyKind::BRep;
                else if (kind == "mesh") body.kind = BodyKind::Mesh;
                else throw std::runtime_error("kind del corpo non valido: " + kind);
                body.visible = field(item, "visible", true);
                body.color = field(item, "color", body.color);
                result.bodies.push_back(std::move(body));
            }
        }
        document = std::move(result);
        error.clear();
        return true;
    } catch (const std::exception& e) {
        error = std::string("Documento non valido: ") + e.what();
        return false;
    }
}

bool save_atomic(const Document& document, const std::filesystem::path& path, std::string& error) {
    if (path.empty() || path.extension() != ".sagomacad") {
        error = "Il percorso deve terminare con .sagomacad";
        return false;
    }
    static std::atomic_uint64_t serial{0};
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    auto temporary = path;
    temporary += ".tmp." + std::to_string(nonce) + "." + std::to_string(serial++);
    try {
        if (!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path());
        {
            std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
            if (!output) throw std::runtime_error("impossibile creare il file temporaneo");
            output << to_json(document).dump(2) << '\n';
            output.flush();
            if (!output) throw std::runtime_error("scrittura incompleta del file temporaneo");
        }
#ifdef _WIN32
        if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            throw std::system_error(GetLastError(), std::system_category(), "sostituzione atomica fallita");
#else
        std::filesystem::rename(temporary, path);
#endif
        error.clear();
        return true;
    } catch (const std::exception& e) {
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        error = e.what();
        return false;
    }
}

bool load(const std::filesystem::path& path, Document& document, std::string& error) {
    try {
        if (path.extension() != ".sagomacad") throw std::runtime_error("Il percorso deve terminare con .sagomacad");
        const auto bytes = std::filesystem::file_size(path);
        if (bytes > 64ULL * 1024ULL * 1024ULL) throw std::runtime_error("Documento troppo grande");
        std::ifstream input(path, std::ios::binary);
        if (!input) throw std::runtime_error("Impossibile aprire il documento");
        const auto value = nlohmann::json::parse(input);
        return from_json(value, document, error);
    } catch (const std::exception& e) {
        error = e.what();
        return false;
    }
}

} // namespace sagomacad::doc
