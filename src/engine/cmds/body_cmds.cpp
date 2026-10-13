#include "engine/command.hpp"
#include "engine/cmds/validation.hpp"
#include <algorithm>

namespace sagomacad::engine::cmds {
std::vector<CommandSpec> body_specs() {
    return {{"body.visibility","Visibilità corpo",{"Browser","Corpi"},std::nullopt,
        "{body: id, visible: bool}",{},
        [](Session& session,const nlohmann::json& params)->CommandResult {
            only_keys(params,{"body","visible"});
            const auto id=string_arg(params,"body",true,256);
            if (!params.contains("visible") || !params["visible"].is_boolean())
                return CommandResult::failure("visible deve essere un booleano");
            auto& bodies=session.document.bodies;
            const auto found=std::find_if(bodies.begin(),bodies.end(),[&](const auto& body){return body.id==id;});
            if (found==bodies.end()) return CommandResult::failure("Corpo sconosciuto: "+id);
            found->visible=params["visible"].get<bool>();
            return {true,{{"body",id},{"visible",found->visible}}, {}};
        },true}};
}
}
