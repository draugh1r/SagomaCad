#include "ui/toolbar_catalog.hpp"

namespace sagomacad::ui {
namespace {
constexpr ToolbarCommand file_menu[] = {
    {"document.new", "Nuovo", "file-plus"},
    {"document.open", "Apri", "folder-open"},
    {"document.save", "Salva", "save"},
    {"document.saveAs", "Salva con nome", "file-pen-line"},
};
constexpr ToolbarCommand edit_actions[] = {
    {"edit.undo", "Annulla", "undo-2"},
    {"edit.redo", "Ripeti", "redo-2"},
};
constexpr ToolbarCommand create[] = {
    {"sketch.create", "Schizzo", "pen-tool"},
    {"feature.extrude", "Estrudi", "move-3d"},
    {"feature.revolve", "Rivoluzione", "rotate-3d"},
    {"feature.hole", "Foro", "circle-dot"},
    {"primitive.add", "Primitiva", "box"},
};
constexpr ToolbarCommand modify[] = {
    {"face.pushPull", "Premi/tira", "move"},
    {"feature.fillet", "Raccordo", "blend"},
    {"feature.chamfer", "Smusso", "diamond"},
    {"param.list", "Cambia parametri", "list"},
};
constexpr ToolbarCommand construct[] = {
    {"construction.plane", "Piano", "between-horizontal-start"},
    {"construction.axis", "Asse", "axis-3d"},
    {"construction.point", "Punto", "crosshair"},
};
constexpr ToolbarCommand inspect[] = {
    {"inspect.measure", "Misura", "ruler-dimension-line"},
    {"inspect.section", "Sezione", "scan-line"},
};
constexpr ToolbarCommand insert[] = {
    {"mesh.import", "Importa mesh", "file-input"},
    {"import.step", "Importa STEP", "folder-input"},
};
constexpr ToolbarCommand select[] = {
    {"selection.set", "Seleziona", "mouse-pointer"},
    {"selection.cycle", "Scorri", "mouse-pointer-2"},
};
constexpr ToolbarCommand export_commands[] = {
    {"export.stl", "STL", "file-output"},
    {"export.3mf", "3MF", "files"},
    {"export.step", "STEP", "package-open"},
};
constexpr ToolbarGroup solid_groups[] = {
    {"CREA", create, 3}, {"MODIFICA", modify, 3},
    {"COSTRUISCI", construct, 3}, {"ISPEZIONA", inspect, 2},
    {"INSERISCI", insert, 2}, {"SELEZIONA", select, 2},
    {"ESPORTA", export_commands, 3},
};
constexpr ToolbarCommand sketch_draw[] = {
    {"sketch.line", "Linea", "slash"},
    {"sketch.rect", "Rettangolo", "rectangle-horizontal"},
    {"sketch.circle", "Cerchio", "circle"},
    {"sketch.arc", "Arco", "radar"},
};
constexpr ToolbarCommand sketch_constrain[] = {
    {"sketch.constraint.add", "Vincolo", "link-2"},
    {"sketch.dimension.add", "Quota", "ruler"},
    {"sketch.dimension.set", "Cambia quota", "pencil-ruler"},
};
constexpr ToolbarCommand sketch_modify[] = {{"sketch.point.move","Sposta punto","move"},{"sketch.dimension.set","Cambia quota","pencil-ruler"}};
constexpr ToolbarCommand sketch_inspect[] = {{"sketch.dimension.add","Quota","ruler"},{"sketch.inspect","Stato solver","scan-search"}};
constexpr ToolbarCommand sketch_insert[] = {{"mesh.import","Importa mesh","file-input"},{"import.step","Importa STEP","folder-input"}};
constexpr ToolbarCommand sketch_finish[] = {{"sketch.finish","Fine schizzo","check"}};
constexpr ToolbarGroup sketch_groups[] = {
    {"CREA", sketch_draw, 3}, {"MODIFICA", sketch_modify, 2}, {"VINCOLI", sketch_constrain, 2},
    {"ISPEZIONA", sketch_inspect, 2}, {"INSERISCI", sketch_insert, 2}, {"SELEZIONA", select, 2}, {"FINE SCHIZZO",sketch_finish,1},
};
constexpr ToolbarTab tabs[] = {
    {"SOLIDO", false, solid_groups},
    {"SCHIZZO", true, sketch_groups},
    {"MESH", false, {}},
    {"GESTISCI", false, {}},
    {"UTILITÀ", false, {}},
};
}
std::span<const ToolbarTab> toolbar_catalog() { return tabs; }
std::span<const ToolbarCommand> file_menu_commands() { return file_menu; }
std::span<const ToolbarCommand> edit_toolbar_commands() { return edit_actions; }
CommandAvailability command_availability(std::string_view id, const engine::Engine* session_engine) {
    if (!session_engine) return {};
    const auto* spec = session_engine->registry().find(id);
    if (!spec) return {};
    try {
        const auto state = spec->enabled ? spec->enabled(session_engine->session()) : engine::Status{};
        return {true, state.ok, state.error};
    } catch (const std::exception& e) {
        return {true, false, e.what()};
    }
}
}
