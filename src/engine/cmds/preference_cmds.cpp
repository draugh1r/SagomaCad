#include "engine/command.hpp"
#include "engine/cmds/validation.hpp"
namespace sagomacad::engine::cmds {
std::vector<CommandSpec> preference_specs(){return {
    {"preferences.get","Leggi preferenze",{"Preferenze"},std::nullopt,"{}",{},
        [](Session& s,const nlohmann::json& p)->CommandResult{only_keys(p,{});return {true,{{"hide_sketch_after_feature",s.hide_sketch_after_feature}}, {}};},false},
    {"preferences.set","Cambia preferenze",{"Preferenze"},std::nullopt,"{hide_sketch_after_feature: boolean}; preferenza locale, default false",{},
        [](Session& s,const nlohmann::json& p)->CommandResult{only_keys(p,{"hide_sketch_after_feature"});if(!p.contains("hide_sketch_after_feature")||!p.at("hide_sketch_after_feature").is_boolean())return CommandResult::failure("hide_sketch_after_feature deve essere booleano");s.hide_sketch_after_feature=p.at("hide_sketch_after_feature").get<bool>();return {true,{{"hide_sketch_after_feature",s.hide_sketch_after_feature}}, {}};},false}
};}
}
