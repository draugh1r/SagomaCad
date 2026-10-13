#include "engine/command.hpp"
#include <iostream>
#include <stdexcept>

using namespace sagomacad::engine;
static void require(bool ok,const char* message) { if (!ok) throw std::runtime_error(message); }
int main() {
    try {
        Engine engine;
        require(engine.execute("param.set",{{"name","larghezza"},{"expression","40 mm"}}).ok,"parametro");
        const auto box=engine.execute("primitive.add",{{"kind","box"},
            {"dimensions",{{"width","larghezza"},{"depth","larghezza"},{"height","20 mm"}}}});
        require(box.ok,"box parametrico");
        require(engine.session().document.timeline.size()==1 && engine.session().document.bodies.size()==1,
                "timeline e corpo");
        const auto& feature=engine.session().document.features.at(box.value.at("feature_id").get<std::string>());
        require(feature.type=="primitive.box" && feature.data["dimensions"]["width"]=="larghezza",
                "espressione non conservata");
        const auto feature_id=box.value.at("feature_id").get<std::string>();
        require(engine.execute("primitive.edit",{{"feature_id",feature_id},{"dimensions",{{"height","25 mm"}}}}).ok,
                "modifica primitiva");
        require(engine.session().document.features.at(feature_id).data["dimensions"]["height"]=="25 mm",
                "modifica espressione non conservata");
        require(!engine.execute("primitive.edit",{{"feature_id",feature_id},{"dimensions",{{"height","-1 mm"}}}}).ok,
                "dimensione negativa accettata");
        require(engine.execute("primitive.add",{{"kind","cylinder"},
            {"dimensions",{{"radius","5 mm"},{"height","20 mm"}}}}).ok,"cilindro");
        require(engine.execute("primitive.add",{{"kind","sphere"},
            {"dimensions",{{"radius","3 mm"}}}}).ok,"sfera");
        require(engine.session().document.timeline.size()==3,"tre feature");
        require(!engine.execute("primitive.add",{{"kind","box"},
            {"dimensions",{{"width","0 mm"},{"depth","5 mm"},{"height","5 mm"}}}}).ok,
            "dimensione nulla accettata");
        require(!engine.execute("primitive.add",{{"kind","cone"},{"dimensions",nlohmann::json::object()}}).ok,
            "kind sconosciuto accettato");
        require(engine.execute("edit.undo").ok && engine.session().document.timeline.size()==2,"undo primitiva");
        require(engine.execute("edit.redo").ok && engine.session().document.timeline.size()==3,"redo primitiva");
        std::cout<<"Primitive: box, cilindro, sfera, validazione e storia verificati\n";
        return 0;
    } catch (const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
