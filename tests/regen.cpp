#include "engine/command.hpp"
#include "regen/regenerator.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace sagomacad;
static void require(bool ok,const char* message) { if (!ok) throw std::runtime_error(message); }
static double volume(const regen::Snapshot& snapshot) {
    require(!snapshot.bodies.empty() && snapshot.bodies[0].exact.has_value(),"corpo senza forma");
    const auto mass=kernel::mass_properties(*snapshot.bodies[0].exact);
    require(static_cast<bool>(mass),"GProp fallito");
    return mass.value->volume;
}
int main() {
    try {
        engine::Engine engine;
        require(engine.execute("param.set",{{"name","larghezza"},{"expression","40 mm"}}).ok,"parametro");
        const auto added=engine.execute("primitive.add",{{"kind","box"},
            {"dimensions",{{"width","larghezza"},{"depth","larghezza"},{"height","20 mm"}}}});
        require(added.ok,"box");
        regen::Regenerator regenerator;
        const auto& initial=regenerator.evaluate(engine.session().document);
        require(std::abs(volume(initial)-32000.0)<1e-6,"volume iniziale 40x40x20");
        require(initial.cache_hits==0,"cache usata al primo giro");
        const auto& repeated=regenerator.evaluate(engine.session().document);
        require(repeated.cache_hits==1 && repeated.features[0].cached,"cache per revisione");
        require(engine.execute("param.set",{{"name","larghezza"},{"expression","60 mm"}}).ok,"modifica parametro");
        const auto& changed=regenerator.evaluate(engine.session().document);
        require(std::abs(volume(changed)-72000.0)<1e-6,"rigenerazione dopo modifica");
        require(changed.cache_hits==0,"cache vecchia non invalidata");
        const auto feature_id=added.value.at("feature_id").get<std::string>();
        engine.session().document.features[feature_id].data["dimensions"]["width"]="missing";
        const auto& failed=regenerator.evaluate(engine.session().document);
        require(!failed.features[0].error.empty() && !failed.bodies[0].exact,
                "feature in errore non isolata");
        require(engine.execute("primitive.add",{{"kind","sphere"},
            {"dimensions",{{"radius","3 mm"}}}}).ok,"feature successiva");
        const auto& continued=regenerator.evaluate(engine.session().document);
        require(continued.features.size()==2 && continued.bodies[1].exact,
                "regen non continua dopo errore");
        std::cout<<"Regen: 32000 → 72000 mm³, cache e errore isolato verificati\n";
        return 0;
    } catch (const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
