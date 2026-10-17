#include "ui/toolbar_catalog.hpp"

namespace sagomacad::ui {
namespace {
constexpr ToolbarCommand file_menu[] = {
    {"document.new", "Nuovo", "file-plus", "new", IconAccent::Solid},
    {"document.open", "Apri", "folder-open", "open", IconAccent::Solid},
    {"document.save", "Salva", "save", "save", IconAccent::Solid},
    {"document.saveAs", "Salva con nome", "file-pen-line", "", IconAccent::Solid},
};
constexpr ToolbarCommand edit_actions[] = {
    {"edit.undo", "Annulla", "undo-2", "undo", IconAccent::Solid},
    {"edit.redo", "Ripeti", "redo-2", "redo", IconAccent::Solid},
};
constexpr ToolbarCommand create[] = {
    {"sketch.create", "Schizzo", "pen-tool", "sketch", IconAccent::Sketch},
    {"feature.extrude", "Estrudi", "move-3d", "extrude", IconAccent::Solid},
    {"feature.revolve", "Rivoluzione", "rotate-3d", "revolve", IconAccent::Solid},
    {"feature.hole", "Foro", "circle-dot", "hole", IconAccent::Solid},
    {"primitive.add", "Primitiva", "box", "box", IconAccent::Solid},
};
constexpr ToolbarCommand modify[] = {
    {"face.pushPull", "Premi/tira", "move", "presspull", IconAccent::Solid},
    {"feature.fillet", "Raccordo", "blend", "fillet", IconAccent::Solid},
    {"feature.chamfer", "Smusso", "diamond", "chamfer", IconAccent::Solid},
    {"param.list", "Cambia parametri", "list", "params", IconAccent::Solid},
};
constexpr ToolbarCommand construct[] = {
    {"construction.plane", "Piano", "between-horizontal-start", "plane", IconAccent::Construction},
    {"construction.axis", "Asse", "axis-3d", "axis", IconAccent::Construction},
    {"construction.point", "Punto", "crosshair", "point", IconAccent::Construction},
};
constexpr ToolbarCommand inspect[] = {
    {"inspect.measure", "Misura", "ruler-dimension-line", "measure", IconAccent::Solid},
    {"inspect.section", "Sezione", "scan-line", "section", IconAccent::Solid},
};
constexpr ToolbarCommand insert[] = {
    {"mesh.import", "Importa mesh", "file-input", "import", IconAccent::Mesh},
    {"import.step", "Importa STEP", "folder-input", "import", IconAccent::Solid},
};
constexpr ToolbarCommand select[] = {
    {"selection.set", "Seleziona", "mouse-pointer", "select", IconAccent::Solid},
    {"selection.cycle", "Scorri", "mouse-pointer-2", "", IconAccent::Solid},
};
constexpr ToolbarCommand export_commands[] = {
    {"export.stl", "STL", "file-output", "export", IconAccent::Mesh},
    {"export.3mf", "3MF", "files", "export", IconAccent::Mesh},
    {"export.step", "STEP", "package-open", "export", IconAccent::Solid},
};
constexpr ToolbarGroup solid_groups[] = {
    {"CREA", create, 3}, {"MODIFICA", modify, 3},
    {"COSTRUISCI", construct, 3}, {"ISPEZIONA", inspect, 2},
    {"INSERISCI", insert, 2}, {"SELEZIONA", select, 2},
    {"ESPORTA", export_commands, 3},
};
constexpr ToolbarCommand sketch_draw[] = {
    {"sketch.line", "Linea", "slash", "line", IconAccent::Sketch},
    {"sketch.rect", "Rettangolo", "rectangle-horizontal", "rect", IconAccent::Sketch},
    {"sketch.circle", "Cerchio", "circle", "circle", IconAccent::Sketch},
    {"sketch.arc", "Arco", "radar", "arc3", IconAccent::Sketch},
};
constexpr ToolbarCommand sketch_constrain[] = {
    {"sketch.constraint.add", "Vincolo", "link-2", "constrainer", IconAccent::Constraint},
    {"sketch.dimension.add", "Quota", "ruler", "dimension", IconAccent::Sketch},
    {"sketch.dimension.set", "Cambia quota", "pencil-ruler", "dimension", IconAccent::Sketch},
};
constexpr ToolbarCommand sketch_modify[] = {{"sketch.point.move", "Sposta punto", "move", "sketch_move", IconAccent::Sketch},{"sketch.dimension.set", "Cambia quota", "pencil-ruler", "dimension", IconAccent::Sketch}};
constexpr ToolbarCommand sketch_inspect[] = {{"sketch.dimension.add", "Quota", "ruler", "dimension", IconAccent::Sketch},{"sketch.inspect", "Stato solver", "scan-search", "inspect", IconAccent::Sketch}};
constexpr ToolbarCommand sketch_insert[] = {{"mesh.import", "Importa mesh", "file-input", "import", IconAccent::Mesh},{"import.step", "Importa STEP", "folder-input", "import", IconAccent::Solid}};
constexpr ToolbarCommand sketch_finish[] = {{"sketch.finish", "Fine schizzo", "check", "finish", IconAccent::Sketch}};
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
const ToolbarCommand* toolbar_command(std::string_view id) {
    for(const auto& c:file_menu)if(c.id==id)return &c;
    for(const auto& c:edit_actions)if(c.id==id)return &c;
    for(const auto& tab:tabs)for(const auto& group:tab.groups)for(const auto& c:group.commands)if(c.id==id)return &c;
    return nullptr;
}
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
