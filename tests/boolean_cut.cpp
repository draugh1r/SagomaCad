#include "engine/command.hpp"
#include <cmath>
#include <stdexcept>

#define require(x) do { if (!(x)) throw std::runtime_error(#x); } while(false)

int main() {
    sagomacad::engine::Engine engine;
    require(engine.execute("param.set",{{"name","larghezza"},{"expression","40 mm"}}).ok);
    const auto box=engine.execute("primitive.add",{{"kind","box"},{"dimensions",{{"width","larghezza"},{"depth","larghezza"},{"height","20 mm"}}}});
    require(box.ok);
    const auto body=box.value.at("body_id").get<std::string>();
    const auto before=engine.execute("model.mass_properties",{{"body",body}});
    require(before.ok && std::abs(before.value.at("volume_mm3").get<double>()-32000.0)<1e-6);
    const auto cut=engine.execute("primitive.add",{{"kind","cylinder"},{"operation","subtract"},{"body",body},
        {"dimensions",{{"radius","5 mm"},{"height","20 mm"}}},
        {"position",{{"x","larghezza / 2"},{"y","larghezza / 2"},{"z","0 mm"}}}});
    require(cut.ok);
    require(engine.session().document.bodies.size()==1);
    const auto expected=32000.0-std::acos(-1.0)*25.0*20.0;
    const auto measured=engine.execute("model.mass_properties",{{"body",body}});
    require(measured.ok && std::abs(measured.value.at("volume_mm3").get<double>()-expected)<1e-4);
    require(engine.execute("edit.undo").ok);
    const auto restored=engine.execute("model.mass_properties",{{"body",body}});
    require(restored.ok && std::abs(restored.value.at("volume_mm3").get<double>()-32000.0)<1e-6);
}
