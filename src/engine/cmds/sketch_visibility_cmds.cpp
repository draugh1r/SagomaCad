#include "engine/command.hpp"
#include "engine/cmds/validation.hpp"
#include "regen/regenerator.hpp"
namespace sagomacad::engine::cmds {
std::vector<CommandSpec> sketch_visibility_specs(){return {{
    "sketch.visibility","Visibilità schizzo",{"Browser","Schizzi"},std::nullopt,
    "{sketch: id, visible: bool|null}; null ripristina la visibilità predefinita",{},
    [](Session& s,const nlohmann::json& p)->CommandResult{
        only_keys(p,{"sketch","visible"});const auto id=string_arg(p,"sketch",true,256);
        auto f=s.document.features.find(id);if(f==s.document.features.end()||f->second.type!="sketch")return CommandResult::failure("Schizzo inesistente: "+id);
        if(!p.contains("visible")||(!p["visible"].is_boolean()&&!p["visible"].is_null()))return CommandResult::failure("visible deve essere un booleano o null");
        if(p["visible"].is_null())f->second.data.erase("visible");else f->second.data["visible"]=p["visible"];
        regen::Regenerator regen;const auto& snapshot=regen.evaluate(s.document);
        return {true,{{"sketch",id},{"visible",regen::sketch_visible(s.document,snapshot,id)},{"visibility",p["visible"]}}, {}};
    },true}};}
}
