#include "engine/command.hpp"
#include "regen/regenerator.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace sagomacad;using J=nlohmann::json;
void check(bool b,const std::string& m){if(!b)throw std::runtime_error(m);}
J run(engine::Engine& e,const char* id,J p=J::object()){auto r=e.execute(id,p);check(r.ok,std::string(id)+": "+r.error);return r.value;}
void close(double a,double b){check(std::abs(a-b)<1e-4,"Volume timeline errato");}
double vol(const regen::Snapshot& s,size_t i){auto m=kernel::mass_properties(*s.bodies.at(i).exact);check(bool(m),m.error);return m.value->volume;}
int main(){try{
    engine::Engine e;auto base=run(e,"primitive.add",{{"kind","box"},{"dimensions",{{"width","2 mm"},{"depth","2 mm"},{"height","2 mm"}}}})["feature_id"];
    run(e,"param.set",{{"name","larghezza"},{"expression","40 mm"}});
    auto sk=run(e,"sketch.create",{{"plane","XY"}})["sketch"];auto rect=run(e,"sketch.rect",{{"width","larghezza"},{"height","40 mm"}});
    auto f=run(e,"feature.extrude",{{"sketch",sk},{"profiles",J::array({rect["entities"]})},{"distance","10 mm"},{"operation","new"}})["feature_id"];
    auto last=run(e,"primitive.add",{{"kind","sphere"},{"dimensions",{{"radius","2 mm"}}}})["feature_id"];
    regen::Regenerator regen;auto snapshot=regen.evaluate(e.session().document);check(snapshot.cache_hits==0,"prima valutazione");close(vol(snapshot,1),16000);
    snapshot=regen.evaluate(e.session().document);check(snapshot.cache_hits==4,"tutto in cache");check(snapshot.hidden_sketches==std::vector<std::string>{sk},"schizzo consumato nascosto");
    run(e,"feature.edit",{{"feature_id",f},{"params",{{"distance","15 mm"}}}});snapshot=regen.evaluate(e.session().document);check(snapshot.cache_hits==2&&snapshot.features[0].cached&&snapshot.features[1].cached&&!snapshot.features[2].cached&&!snapshot.features[3].cached,"rivalutazione dalla feature modificata");close(vol(snapshot,1),24000);
    run(e,"param.set",{{"name","larghezza"},{"expression","60 mm"}});snapshot=regen.evaluate(e.session().document);check(snapshot.cache_hits==1,"parametro invalida solo dal primo dipendente");close(vol(snapshot,1),36000);
    run(e,"param.set",{{"name","estraneo"},{"expression","20 mm"}});check(regen.evaluate(e.session().document).cache_hits==4,"parametro estraneo non invalida");
    run(e,"timeline.rollback",{{"index",2}});snapshot=regen.evaluate(e.session().document);check(snapshot.features.size()==2&&!snapshot.bodies[1].exact&&snapshot.hidden_sketches.empty(),"rollback prima dell'estrusione");
    run(e,"timeline.rollback",{{"index",4}});check(regen.evaluate(e.session().document).cache_hits==4,"avanzamento riusa il suffisso invariato");
    run(e,"feature.suppress",{{"feature_id",f},{"suppressed",true}});snapshot=regen.evaluate(e.session().document);check(!snapshot.bodies[1].exact&&snapshot.bodies[2].exact&&snapshot.hidden_sketches.empty(),"soppressione e prosecuzione");run(e,"edit.undo");close(vol(regen.evaluate(e.session().document),1),36000);
    auto dimensions=run(e,"sketch.inspect",{{"sketch",sk}})["dimensions"];run(e,"sketch.dimension.set",{{"sketch",sk},{"dimension",dimensions[1]["id"]},{"expression","20 mm"}});close(vol(regen.evaluate(e.session().document),1),18000);
    run(e,"feature.delete",{{"feature_id",sk}});snapshot=regen.evaluate(e.session().document);check(snapshot.features[1].id==f.get<std::string>()&&!snapshot.features[1].error.empty()&&snapshot.features[1].error.find("Schizzo sorgente mancante")!=std::string::npos,"errore chiaro per sorgente eliminata");check(snapshot.bodies[2].exact.has_value(),"valutazione prosegue dopo errore");check(!run(e,"feature.get",{{"feature_id",f}})["error"].get<std::string>().empty(),"errore accessibile agli agenti");run(e,"edit.undo");close(vol(regen.evaluate(e.session().document),1),18000);
    const auto before=e.session().document;check(!e.execute("feature.edit",{{"feature_id",f},{"params",{{"distance","0 mm"}}}}).ok,"modifica errata rifiutata");check(e.session().document==before,"modifica errata atomica");
    run(e,"timeline.rollback",{{"index",1}});run(e,"feature.delete",{{"feature_id",last}});check(e.session().document.rollback==1,"cancellazione oltre marker lo conserva");
    check(!e.execute("timeline.rollback",{{"index",-1}}).ok&&!e.execute("timeline.rollback",{{"index",999}}).ok,"limiti marker");
    std::cout<<"Timeline incrementale: prefisso conservato, rollback, quote a monte, soppressione, errore isolato e ripristino OK\n";
}catch(const std::exception& e){std::cerr<<e.what();return 1;}}
