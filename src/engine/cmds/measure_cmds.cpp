#include "engine/command.hpp"
#include "engine/cmds/validation.hpp"
#include "regen/regenerator.hpp"
#include <algorithm>

namespace sagomacad::engine::cmds {
std::vector<CommandSpec> measure_specs() {
    return {{"model.mass_properties","Proprietà di massa",{"Ispeziona"},std::nullopt,
        "{body?: id, stage?: exact|final}; volume mm³, area mm², baricentro e limiti mm",
        [](const Session& session){return session.document.bodies.empty()?Status::failure("Nessun corpo da misurare"):Status{};},
        [](Session& session,const nlohmann::json& params)->CommandResult {
            only_keys(params,{"body","stage"});
            const auto body=params.contains("body")?string_arg(params,"body",true,256):session.document.bodies.front().id;
            const auto stage=params.contains("stage")?string_arg(params,"stage",true,16):"final";
            if (stage!="exact" && stage!="final") return CommandResult::failure("stage deve essere exact o final");
            regen::Regenerator regenerator;
            const auto& snapshot=regenerator.evaluate(session.document);
            const auto found=std::find_if(snapshot.bodies.begin(),snapshot.bodies.end(),[&](const auto& value){return value.id==body;});
            if (found==snapshot.bodies.end()) return CommandResult::failure("Corpo sconosciuto: "+body);
            const auto& shape=stage=="exact"?found->exact:found->final;
            if (!shape) return CommandResult::failure(found->error.empty()?"Il corpo non contiene una forma":found->error);
            const auto mass=kernel::mass_properties(*shape);
            if (!mass) return CommandResult::failure(mass.error);
            const auto point=[](kernel::Vec3 v){return nlohmann::json{{"x",v.x},{"y",v.y},{"z",v.z}};};
            return {true,{{"body",body},{"stage",stage},{"volume_mm3",mass.value->volume},
                          {"area_mm2",mass.value->area},{"centroid_mm",point(mass.value->center)},
                          {"bbox_mm",{{"min",point(mass.value->minimum)},{"max",point(mass.value->maximum)}}}}, {}};
        },false}};
}
} // namespace sagomacad::engine::cmds
