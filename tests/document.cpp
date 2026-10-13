#include "doc/document.hpp"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>

using namespace sagomacad::doc;

static void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

int main() {
    try {
        const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
        const auto path = std::filesystem::temp_directory_path() /
                          ("sagomacad-document-" + std::to_string(nonce) + ".sagomacad");
        Document source;
        source.name = "Prova àèé";
        source.parameters = {{"larghezza", "40 mm", "mm", "Lato esterno"},
                             {"foro", "larghezza / 4", "mm", ""}};
        Feature feature;
        feature.id = "f1";
        feature.name = "Schizzo";
        feature.type = "sketch";
        feature.kind = FeatureKind::Exact;
        feature.data = {{"plane", "XY"}};
        source.features.emplace(feature.id, feature);
        source.timeline.push_back(feature.id);
        source.rollback = 1;
        source.bodies = {{"b1", "Corpo", BodyKind::BRep, true, "#AABBCC"},
                         {"b2", "Mesh", BodyKind::Mesh, false, "#334455"}};

        std::string error;
        require(save_atomic(source, path, error), error.c_str());
        Document restored;
        require(load(path, restored, error), error.c_str());
        require(restored == source, "salva/rileggi non conserva il documento");
        source.name = "Seconda versione";
        require(save_atomic(source, path, error), error.c_str());
        require(load(path, restored, error), error.c_str());
        require(restored == source, "la sostituzione atomica non ha aggiornato il documento");

        Document old;
        require(from_json({{"parameters", nlohmann::json::array({{{"name", "x"}}})},
                           {"bodies", nlohmann::json::array({{{"id", "b"}}})},
                           {"features", {{"f", {{"id", "f"}}}}}}, old, error), error.c_str());
        require(old.parameters[0].expression.empty() && old.parameters[0].unit.empty() &&
                old.parameters[0].comment.empty(), "default parametro");
        require(old.bodies[0].kind == BodyKind::BRep && old.bodies[0].visible, "default corpo");
        require(old.features.at("f").kind == FeatureKind::Exact, "default feature");
        require(old.timeline.empty() && old.rollback == 0, "default timeline");

        Document invalid;
        require(!from_json({{"bodies", nlohmann::json::array({{{"kind", "invalid"}}})}}, invalid, error),
                "kind non valido accettato");
        require(!error.empty(), "errore non leggibile");
        std::filesystem::remove(path);
        std::cout << "Documento: roundtrip, sostituzione atomica e default verificati\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
