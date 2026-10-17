#include "regen/extrusion.hpp"
#include "doc/expression.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace sagomacad::regen {
kernel::Result<kernel::Shape> sweep_profile(const doc::Document& d,const nlohmann::json& data,const Snapshot& snapshot,kernel::Shape face,kernel::Vec3 normal,kernel::Vec3 origin)noexcept{
    try{
        auto value=[&](const char* key){doc::Quantity q;std::string error;if(!doc::evaluate_expression(d,data.at(key).get<std::string>(),q,error))throw std::runtime_error(std::string(key)+": "+error);
            if((q.dimension!=doc::Dimension::Length&&q.dimension!=doc::Dimension::Scalar)||!std::isfinite(q.value)||std::abs(q.value)>1e7)throw std::runtime_error(std::string(key)+": lunghezza non valida");return q.value;};
        double distance=value("distance"),start=0;
        auto direction=data.value("direction",std::string("one_side")),extent=data.value("extent",std::string("distance"));
        if(direction!="one_side"&&direction!="symmetric"&&direction!="two_sides")throw std::runtime_error("Direzione non valida");
        if(extent!="distance"&&extent!="through_all")throw std::runtime_error("Estensione non valida");
        if(extent=="through_all"){
            if(data.value("operation",std::string("new"))=="new")throw std::runtime_error("Attraverso tutto richiede corpi di destinazione");
            double low=0,high=0;bool target=false;
            for(const auto& body:snapshot.bodies)if(body.exact&&std::find(data.at("target_bodies").begin(),data.at("target_bodies").end(),body.id)!=data.at("target_bodies").end()){
                auto mass=kernel::mass_properties(*body.exact);if(!mass)throw std::runtime_error(mass.error);target=true;
                auto lo=mass.value->minimum,hi=mass.value->maximum;
                for(int i=0;i<8;++i){kernel::Vec3 p{i&1?hi.x:lo.x,i&2?hi.y:lo.y,i&4?hi.z:lo.z};double t=(p.x-origin.x)*normal.x+(p.y-origin.y)*normal.y+(p.z-origin.z)*normal.z;low=std::min(low,t);high=std::max(high,t);}
            }
            if(!target)throw std::runtime_error("Nessun corpo di destinazione disponibile");double margin=std::max(1.,(high-low)*.01);
            if(direction=="one_side"){if(distance<0){start=low-margin;distance=-start;}else distance=high+margin;}
            else{start=low-margin;distance=high-low+2*margin;}
        }else if(direction=="symmetric")start=-distance/2;
        else if(direction=="two_sides"){double second=value("second_distance");if(distance<=0||second<0)throw std::runtime_error("Le distanze dei due lati devono essere positive");start=-second;distance+=second;}
        if(std::abs(distance)<1e-7)throw std::runtime_error("La distanza deve essere diversa da zero");
        auto shifted=kernel::translate(face,normal.x*start,normal.y*start,normal.z*start);if(!shifted)throw std::runtime_error(shifted.error);
        return kernel::extrude(*shifted.value,{normal.x*distance,normal.y*distance,normal.z*distance});
    }catch(const std::exception& e){return kernel::Result<kernel::Shape>::failure(e.what());}catch(...){return kernel::Result<kernel::Shape>::failure("Estrusione fallita");}
}
}
