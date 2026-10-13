#include "engine/command.hpp"
#include "engine/cmds/validation.hpp"
#include "regen/regenerator.hpp"
#include "mesh/surface.hpp"
#include <algorithm>
#include <cmath>

namespace sagomacad::engine::cmds {
std::vector<CommandSpec> export_specs() {
    return {{"export.stl","Esporta STL",{"File","Esporta"},std::nullopt,
        "{path: string, body?: id, tolerance?: number in mm, stage?: exact|final}",
        [](const Session& session) { return session.document.bodies.empty()?Status::failure("Nessun corpo da esportare"):Status{}; },
        [](Session& session,const nlohmann::json& params)->CommandResult {
            only_keys(params,{"path","body","tolerance","stage"});
            const auto path=string_arg(params,"path",true,4096);
            const auto body=params.contains("body")?string_arg(params,"body",true,256):session.document.bodies.front().id;
            const auto stage=params.contains("stage")?string_arg(params,"stage",true,16):"final";
            if (stage!="exact" && stage!="final") return CommandResult::failure("stage deve essere exact o final");
            double tolerance=0.1;
            if (params.contains("tolerance")) {
                if (!params["tolerance"].is_number()) return CommandResult::failure("tolerance deve essere un numero");
                tolerance=params["tolerance"].get<double>();
            }
            if (!std::isfinite(tolerance) || tolerance<0.001 || tolerance>10.0)
                return CommandResult::failure("Tolleranza STL fuori intervallo (0.001–10 mm)");
            regen::Regenerator regenerator;
            const auto& snapshot=regenerator.evaluate(session.document);
            const auto found=std::find_if(snapshot.bodies.begin(),snapshot.bodies.end(),[&](const auto& b){return b.id==body;});
            if (found==snapshot.bodies.end()) return CommandResult::failure("Corpo sconosciuto: "+body);
            const auto& shape=stage=="exact"?found->exact:found->final;
            if (!shape) return CommandResult::failure(found->error.empty()?"Lo stadio richiesto non contiene una forma":found->error);
            auto surface=mesh::build(*shape,tolerance);
            if (!surface) return CommandResult::failure(surface.error);
            auto result=mesh::write_stl(*surface.value,path);
            if (!result) return CommandResult::failure(result.error);
            return {true,{{"path",path},{"body",body},{"stage",stage},{"triangles",*result.value}}, {}};
        },false}};
}
}
