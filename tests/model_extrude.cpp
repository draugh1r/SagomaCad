#include "engine/command.hpp"
#include "regen/regenerator.hpp"
#include <cmath>
#include <numbers>
#include <iostream>
#include <stdexcept>
using namespace sagomacad;
using J=nlohmann::json;
void check(bool b,const std::string& m){if(!b)throw std::runtime_error(m);}
J run(engine::Engine& e,const char* id,J p=J::object()){auto r=e.execute(id,p);check(r.ok,std::string(id)+": "+r.error);return r.value;}
double volume(engine::Engine& e,const std::string& body){return run(e,"model.mass_properties",{{"body",body}})["volume_mm3"].get<double>();}
void close(double a,double b){check(std::abs(a-b)<1e-4,"volume: "+std::to_string(a)+" != "+std::to_string(b));}
J rectangle(engine::Engine& e,double x=0){run(e,"sketch.create",{{"plane","XY"}});auto r=run(e,"sketch.rect",{{"x",x},{"width","10 mm"},{"height","10 mm"}});return J::array({r["entities"]});}
int main(){try{
    engine::Engine e;run(e,"param.set",{{"name","larghezza"},{"expression","60 mm"}});run(e,"param.set",{{"name","foro"},{"expression","10 mm"}});
    auto sk=run(e,"sketch.create",{{"plane","XY"}})["sketch"];auto rect=run(e,"sketch.rect",{{"width","larghezza"},{"height","40 mm"},{"centered",true}});run(e,"sketch.circle",{{"diameter","foro"}});
    auto inspect=run(e,"sketch.inspect");J ref;for(const auto& r:inspect["regions"])if(r["boundary"].size()==4)ref=r["id"];
    auto plate=run(e,"feature.extrude",{{"sketch",sk},{"profiles",J::array({ref})},{"distance","10 mm"}});auto body=plate["body_id"].get<std::string>();
    close(volume(e,body),(2400-25*std::numbers::pi)*10);
    run(e,"param.set",{{"name","larghezza"},{"expression","80 mm"}});run(e,"param.set",{{"name","foro"},{"expression","20 mm"}});close(volume(e,body),(3200-100*std::numbers::pi)*10);
    run(e,"edit.undo");close(volume(e,body),(3200-25*std::numbers::pi)*10);run(e,"edit.redo");
    for(auto op:{"new","join","cut","intersect"}){
        engine::Engine t;auto base=run(t,"primitive.add",{{"kind","box"},{"dimensions",{{"width","20 mm"},{"depth","20 mm"},{"height","10 mm"}}}})["body_id"].get<std::string>();
        auto refs=rectangle(t,10);J p={{"sketch",t.session().active_sketch},{"profiles",refs},{"distance","15 mm"},{"operation",op}};if(std::string(op)!="new")p["target_bodies"]=J::array({base});
        auto result=run(t,"feature.extrude",p);close(volume(t,result["body_id"]),std::string(op)=="new"?1500:std::string(op)=="join"?4500:std::string(op)=="cut"?3000:1000);
    }
    for(auto dir:{"one_side","symmetric","two_sides"}){
        engine::Engine t;auto refs=rectangle(t);auto f=run(t,"feature.extrude",{{"sketch",t.session().active_sketch},{"profiles",refs},{"distance","10 mm"},{"second_distance","6 mm"},{"direction",dir}});
        auto mass=run(t,"model.mass_properties");close(mass["volume_mm3"],std::string(dir)=="two_sides"?1600:1000);
        close(mass["bbox_mm"]["min"]["z"],std::string(dir)=="symmetric"?-5:std::string(dir)=="two_sides"?-6:0);
    }
    engine::Engine t;auto base=run(t,"primitive.add",{{"kind","box"},{"dimensions",{{"width","20 mm"},{"depth","20 mm"},{"height","50 mm"}}}})["body_id"].get<std::string>();
    auto refs=rectangle(t);run(t,"feature.extrude",{{"sketch",t.session().active_sketch},{"profiles",refs},{"extent","through_all"},{"operation","cut"},{"target_bodies",J::array({base})}});close(volume(t,base),15000);
    const auto before=t.session().document;check(!t.execute("feature.extrude",{{"sketch","missing"},{"profiles",refs}}).ok,"sorgente mancante rifiutata");check(t.session().document==before,"errore atomico");
    check(!t.execute("feature.extrude",{{"sketch",t.session().active_sketch},{"profiles",J::array({"missing"})}}).ok,"profilo mancante rifiutato");
    engine::Engine multi;
    std::vector<std::string> bodies;
    for(auto x:{0,30})bodies.push_back(run(multi,"primitive.add",{{"kind","box"},{"position",{{"x",std::to_string(x)+" mm"}}},{"dimensions",{{"width","20 mm"},{"depth","20 mm"},{"height","10 mm"}}}})["body_id"]);
    auto source=run(multi,"sketch.create",{{"plane","XY"}})["sketch"];auto wide=run(multi,"sketch.rect",{{"width","50 mm"},{"height","5 mm"}});
    run(multi,"feature.extrude",{{"sketch",source},{"profiles",J::array({wide["entities"]})},{"extent","through_all"},{"operation","cut"},{"target_bodies",bodies}});
    for(const auto& b:bodies)close(volume(multi,b),3000);
    engine::Engine joined;
    std::vector<std::string> joined_bodies;
    for(auto x:{0,20})joined_bodies.push_back(run(joined,"primitive.add",{{"kind","box"},{"position",{{"x",std::to_string(x)+" mm"}}},{"dimensions",{{"width","10 mm"},{"depth","10 mm"},{"height","10 mm"}}}})["body_id"]);
    auto join_source=run(joined,"sketch.create",{{"plane","XY"}})["sketch"];auto bridge=run(joined,"sketch.rect",{{"x",5},{"width","20 mm"},{"height","10 mm"}});
    run(joined,"feature.extrude",{{"sketch",join_source},{"profiles",J::array({bridge["entities"]})},{"distance","10 mm"},{"operation","join"},{"target_bodies",joined_bodies}});
    close(volume(joined,joined_bodies[0]),3000);auto merged=regen::Regenerator{}.evaluate(joined.session().document);check(!merged.bodies[1].exact,"unione multipla produce un solo corpo senza materiale duplicato");
    engine::Engine negative;auto negative_refs=rectangle(negative);run(negative,"feature.extrude",{{"sketch",negative.session().active_sketch},{"profiles",negative_refs},{"distance","-5 mm"}});auto nm=run(negative,"model.mass_properties");close(nm["volume_mm3"],500);close(nm["bbox_mm"]["min"]["z"],-5);
    engine::Engine plane;auto vertical=run(plane,"sketch.create",{{"plane","XZ"}})["sketch"];auto side=run(plane,"sketch.rect",{{"width","10 mm"},{"height","10 mm"}});
    run(plane,"feature.extrude",{{"sketch",vertical},{"profiles",J::array({side["entities"]})},{"distance","5 mm"}});auto pm=run(plane,"model.mass_properties");close(pm["volume_mm3"],500);close(pm["bbox_mm"]["min"]["y"],-5);
    std::cout<<"Estrusione: corona parametrica, quattro operazioni, direzioni, attraverso tutto, più corpi e piano XZ OK\n";
}catch(const std::exception& e){std::cerr<<e.what();return 1;}}
