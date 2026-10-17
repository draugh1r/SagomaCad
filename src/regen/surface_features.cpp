#include "regen/surface_features.hpp"
#include "kernel/topology.hpp"
#include "doc/expression.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace sagomacad::regen {
Regenerator::Cached build_surface_feature(const doc::Document& d,const doc::Feature& f,const Snapshot& snapshot){
    try{
        auto body_id=f.data.at("body_id").get<std::string>();auto body=std::find_if(snapshot.bodies.begin(),snapshot.bodies.end(),[&](const auto& b){return b.id==body_id;});
        if(body==snapshot.bodies.end()||!body->exact)throw std::runtime_error("Corpo sorgente assente: "+body_id);
        auto length=[&](const char* key){doc::Quantity q;std::string error;if(!doc::evaluate_expression(d,f.data.at(key).get<std::string>(),q,error))throw std::runtime_error(std::string(key)+": "+error);
            if((q.dimension!=doc::Dimension::Length&&q.dimension!=doc::Dimension::Scalar)||!std::isfinite(q.value)||q.value<=0||q.value>1e7)throw std::runtime_error(std::string(key)+": lunghezza positiva richiesta");return q.value;};
        std::vector<int> ids;std::string names;
        const auto& refs=f.data.at("edges");if(!refs.is_array()||refs.empty()||refs.size()>4096)throw std::runtime_error("Seleziona uno o più spigoli");
        for(const auto& input:refs){auto ref=input.get<doc::TopologyRef>();if(ref.body!=body_id)throw std::runtime_error("Gli spigoli devono appartenere allo stesso corpo");auto edge=kernel::resolve(*body->exact,ref,"edge");if(!edge)throw std::runtime_error(edge.error);ids.push_back(edge.value->id);names+=" ["+ref.name+"]";}
        auto result=f.type=="feature.fillet"?kernel::fillet(*body->exact,ids,length("radius"),f.id):kernel::chamfer(*body->exact,ids,length("distance"),f.id);
        if(!result)throw std::runtime_error(names+": "+result.error);return {result.value,{}};
    }catch(const std::exception& e){return {{},e.what()};}catch(...){return {{},"Operazione sugli spigoli fallita"};}
}
}
