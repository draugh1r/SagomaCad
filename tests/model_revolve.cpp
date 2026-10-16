#include "engine/command.hpp"
#include <cmath>
#include <numbers>
#include <iostream>
#include <stdexcept>
using namespace sagomacad;using J=nlohmann::json;
void check(bool b,const std::string& m){if(!b)throw std::runtime_error(m);}
J run(engine::Engine& e,const char* id,J p=J::object()){auto r=e.execute(id,p);check(r.ok,std::string(id)+": "+r.error);return r.value;}
void close(double a,double b){check(std::abs(a-b)<1e-4,"Volume rivoluzione: "+std::to_string(a)+" != "+std::to_string(b));}
int main(){try{
    for(bool line_axis:{false,true})for(auto type:{"full","angle"}){
        engine::Engine e;run(e,"param.set",{{"name","altezza"},{"expression","10 mm"}});run(e,"param.set",{{"name","angolo"},{"expression","90 deg"}});
        auto sk=run(e,"sketch.create",{{"plane","XZ"}})["sketch"];auto rect=run(e,"sketch.rect",{{"x",5},{"width","10 mm"},{"height","altezza"}});
        std::string axis="Z";if(line_axis)axis=run(e,"sketch.line",{{"x1",0},{"y1",0},{"x2",0},{"y2",30},{"construction",true}})["entity"];
        auto f=run(e,"feature.revolve",{{"sketch",sk},{"profiles",J::array({rect["entities"]})},{"axis",axis},{"type",type},{"angle","angolo"}});
        double factor=std::string(type)=="full"?1:.25;close(run(e,"model.mass_properties")["volume_mm3"],2000*std::numbers::pi*factor);
        run(e,"param.set",{{"name","altezza"},{"expression","20 mm"}});close(run(e,"model.mass_properties")["volume_mm3"],4000*std::numbers::pi*factor);
        if(std::string(type)=="angle"){run(e,"param.set",{{"name","angolo"},{"expression","180 deg"}});close(run(e,"model.mass_properties")["volume_mm3"],2000*std::numbers::pi);}
        const auto before=e.session().document;
        check(!e.execute("feature.revolve",{{"sketch",sk},{"profiles",J::array({rect["entities"]})},{"axis","missing"}}).ok,"asse sconosciuto rifiutato");check(e.session().document==before,"rivoluzione fallita atomica");
        for(auto angle:{"0 deg","361 deg","10 mm","1e309 deg"})check(!e.execute("feature.revolve",{{"sketch",sk},{"profiles",J::array({rect["entities"]})},{"axis",axis},{"angle",angle}}).ok,"angolo errato rifiutato");
    }
    for(auto op:{"join","cut","intersect"}){
        engine::Engine e;auto body=run(e,"primitive.add",{{"kind","cylinder"},{"dimensions",{{"radius","10 mm"},{"height","10 mm"}}}})["body_id"];
        auto sk=run(e,"sketch.create",{{"plane","XZ"}})["sketch"];auto rect=run(e,"sketch.rect",{{"x",5},{"width","10 mm"},{"height","10 mm"}});
        run(e,"feature.revolve",{{"sketch",sk},{"profiles",J::array({rect["entities"]})},{"axis","Z"},{"type","full"},{"operation",op},{"target_bodies",J::array({body})}});
        close(run(e,"model.mass_properties")["volume_mm3"],std::numbers::pi*(std::string(op)=="join"?2250:std::string(op)=="cut"?250:750));
    }
    std::cout<<"Rivoluzione: anello esatto 6283,185307 mm3, 90/180/360 gradi, asse origine/linea, operazioni OK\n";
}catch(const std::exception& e){std::cerr<<e.what();return 1;}}
