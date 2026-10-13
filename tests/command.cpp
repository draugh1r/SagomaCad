#include "engine/command.hpp"
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>

using namespace sagomacad::engine;
static void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
static void good(const CommandResult& result) {
    if (!result.ok) throw std::runtime_error(result.error);
}
static double value(const Engine& engine, const char* name) {
    for (const auto& row : evaluated_parameters_json(engine.session().document))
        if (row.at("name") == name) return row.at("value").get<double>();
    throw std::runtime_error(std::string("parametro assente: ") + name);
}

int main() {
    try {
        Engine engine;
        require(engine.registry().find("param.set") && engine.registry().find("edit.undo"), "registro incompleto");
        require(!engine.execute("edit.undo").ok, "undo attivo sulla storia vuota");
        good(engine.execute("param.set", {{"name", "larghezza"}, {"expression", "40 mm"}}));
        good(engine.execute("param.set", {{"name", "foro"}, {"expression", "larghezza / 4"}}));
        require(std::abs(value(engine, "foro") - 10.0) < 1e-9, "parametro collegato");
        require(!engine.execute("param.delete", {{"name", "larghezza"}}).ok, "eliminazione di una dipendenza");

        const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
        const auto path = std::filesystem::temp_directory_path() /
                          ("sagomacad-command-" + std::to_string(nonce) + ".sagomacad");
        good(engine.execute("document.saveAs", {{"path", path.string()}}));
        good(engine.execute("document.new"));
        require(engine.session().document.parameters.empty(), "document.new non svuota il documento");
        good(engine.execute("document.open", {{"path", path.string()}}));
        require(std::abs(value(engine, "foro") - 10.0) < 1e-9, "riapertura");
        good(engine.execute("param.set", {{"name", "larghezza"}, {"expression", "80 mm"}}));
        require(std::abs(value(engine, "foro") - 20.0) < 1e-9, "aggiornamento della dipendenza");
        good(engine.execute("edit.undo"));
        require(std::abs(value(engine, "foro") - 10.0) < 1e-9, "undo");
        good(engine.execute("edit.redo"));
        require(std::abs(value(engine, "foro") - 20.0) < 1e-9, "redo");

        const auto steps = engine.session().history.size();
        good(engine.execute("param.set", {{"name", "larghezza"}, {"expression", "90 mm"}}, "drag-1"));
        good(engine.execute("param.set", {{"name", "larghezza"}, {"expression", "100 mm"}}, "drag-1"));
        require(engine.session().history.size() == steps + 1, "trascinamento non coalesciuto");
        good(engine.execute("edit.undo"));
        require(std::abs(value(engine, "foro") - 20.0) < 1e-9, "undo del trascinamento");
        require(!engine.execute("param.set", {{"name", 12}, {"expression", "1 mm"}}).ok,
                "tipo errato accettato");
        require(!engine.execute("comando.inesistente").ok, "id inesistente accettato");
        std::filesystem::remove(path);
        std::cout << "Comandi: file, parametri collegati, undo/redo e coalescenza verificati\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
