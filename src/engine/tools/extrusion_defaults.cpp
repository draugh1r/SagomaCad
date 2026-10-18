#include "engine/tools/extrusion_defaults.hpp"
#include "regen/regenerator.hpp"
#include "regen/model_features.hpp"
#include "regen/attached_sketch.hpp"
#include "kernel/measure.hpp"
#include "solver/sketch_solver.hpp"
#include "solver/profile_regions.hpp"
#include <algorithm>
namespace sagomacad::engine {
CommandResult extrusion_defaults(const doc::Document& document,const nlohmann::json& params){
    try{
        using J=nlohmann::json;
        regen::Regenerator regen;const auto snapshot=regen.evaluate(document);
        if(std::none_of(snapshot.bodies.begin(),snapshot.bodies.end(),[](const auto& b){return bool(b.exact);}))return {true,{{"operation","new"},{"target_bodies",J::array()}},{}};
        auto data=params;data["operation"]="new";data["extent"]="distance";
        if(!data.contains("distance"))data["distance"]="10 mm";
        if(!data.contains("second_distance"))data["second_distance"]="10 mm";
        std::vector<kernel::Shape> profiles;
        if(data.contains("face")){
            auto ref=data.at("face").get<doc::TopologyRef>();
            auto b=std::find_if(snapshot.bodies.begin(),snapshot.bodies.end(),[&](const auto& b){return b.id==ref.body;});
            if(b==snapshot.bodies.end()||!b->exact)throw std::runtime_error("Corpo sorgente assente");
            auto face=kernel::resolve(*b->exact,ref,"face");if(!face)throw std::runtime_error(face.error);
            auto shape=kernel::face_shape(*b->exact,face.value->id);if(!shape)throw std::runtime_error(shape.error);profiles.push_back(*shape.value);
        }else{
            auto id=data.at("sketch").get<std::string>();
            auto solved=solver::solve(document,regen::evaluated_sketch(document,id));
            if(!solved.error.empty())throw std::runtime_error(solved.error);
            for(const auto& ref:data.at("profiles")){
                const auto regions=solver::profile_regions(solved.sketch,solved.profiles);
                auto region=std::find_if(regions.begin(),regions.end(),[&](const auto& r){if(ref.is_string())return ref==r.id;if(ref.is_array()){auto a=ref.get<std::vector<std::string>>(),b=r.boundary;std::sort(a.begin(),a.end());std::sort(b.begin(),b.end());return a==b;}return false;});
                if(region==regions.end())throw std::runtime_error("Profilo non disponibile");
                auto shape=kernel::profile_face(solved.sketch,region->boundary,region->holes);if(!shape)throw std::runtime_error(shape.error);profiles.push_back(*shape.value);
            }
        }
        doc::Feature f;f.id="operation-probe";f.type="feature.extrude";f.data=data;
        const auto swept=regen::build_model_feature(document,f,snapshot);if(!swept.shape)throw std::runtime_error(swept.error);
        J touching=J::array(),overlapping=J::array();
        const auto mass=kernel::mass_properties(*swept.shape);if(!mass)throw std::runtime_error(mass.error);
        for(const auto& body:snapshot.bodies)if(body.exact){
            bool contact=false,inward=false;
            for(const auto& profile:profiles){auto r=kernel::extrusion_contact(profile,*swept.shape,*body.exact);if(!r)throw std::runtime_error(r.error);contact|=r.value->distance<=1e-7;inward|=r.value->distance<=1e-7&&r.value->overlap_volume>std::max(1e-9,mass.value->volume*1e-8);}
            if(contact)touching.push_back(body.id);if(inward)overlapping.push_back(body.id);
        }
        // Entering material takes precedence; targets contain only the bodies
        // involved in the chosen operation, never every body in the document.
        return {true,{{"operation",!overlapping.empty()?"cut":!touching.empty()?"join":"new"},{"target_bodies",!overlapping.empty()?overlapping:touching}}, {}};
    }catch(const std::exception& e){return CommandResult::failure(e.what());}
}
}
