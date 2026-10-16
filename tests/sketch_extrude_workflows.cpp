#include "ui/model_editor.hpp"
#include "regen/regenerator.hpp"
#include <cmath>
#include <cstdio>
#include <iostream>
#include <stdexcept>
using namespace sagomacad;using J=nlohmann::json;
void check(bool b,const std::string& m){if(!b)throw std::runtime_error(m);}
J run(engine::Engine& e,const char* id,J p=J::object()){auto r=e.execute(id,p);check(r.ok,std::string(id)+": "+r.error);return r.value;}
double volume(engine::Engine& e,const J& body){return run(e,"model.mass_properties",{{"body",body}})["volume_mm3"];}
void close(double a,double b){check(std::abs(a-b)<1e-4,"volume errato: "+std::to_string(a));}
int main(){try{
    engine::Engine e;ui::State state;
    run(e,"param.set",{{"name","larghezza"},{"expression","40 mm"}});
    auto first=run(e,"sketch.create",{{"plane","XY"}})["sketch"].get<std::string>();
    auto rectangle=run(e,"sketch.rect",{{"width","larghezza"},{"height","20 mm"}});
    auto second=run(e,"sketch.create",{{"plane","XY"}})["sketch"].get<std::string>();
    run(e,"sketch.circle",{{"x",10},{"y",10},{"diameter","4 mm"}});run(e,"sketch.finish");
    auto refs=J::array({rectangle["entities"]});
    auto plate=run(e,"feature.extrude",{{"sketch",first},{"profiles",refs},{"distance","10 mm"}});
    close(volume(e,plate["body_id"]),8000);
    check(!run(e,"sketch.inspect",{{"sketch",first}})["visible"].get<bool>(),"sorgente nascosta automaticamente");
    check(run(e,"sketch.inspect",{{"sketch",second}})["visible"].get<bool>(),"altro schizzo conservato");
    run(e,"sketch.visibility",{{"sketch",first},{"visible",true}});
    run(e,"sketch.edit",{{"sketch",first}});auto dim=run(e,"sketch.inspect")["dimensions"][0]["id"];
    run(e,"sketch.dimension.set",{{"dimension",dim},{"expression","larghezza"}});run(e,"sketch.finish");
    check(run(e,"sketch.inspect",{{"sketch",first}})["visibility"]==true,"visibilità conservata modificando quote");
    state.selected_profile_sketch=first;ui::begin_model(state,e,false);
    check(state.model.sketch==first&&state.model.profiles.size()==1,"riuso sorgente dal browser con profilo unico");
    state.model.operation=0;state.model.targets={plate["body_id"].get<std::string>()};std::snprintf(state.model.distance.data(),state.model.distance.size(),"15 mm");
    check(ui::submit_model(state,e),state.model.error);close(volume(e,plate["body_id"]),12000);
    auto joined=e.session().document.timeline.back();
    run(e,"param.set",{{"name","larghezza"},{"expression","60 mm"}});close(volume(e,plate["body_id"]),18000);
    ui::begin_model(state,e,false,joined);check(state.model.feature_id==joined&&state.model.profiles.size()==1,"riapertura estrusione da riferimenti alle entità");
    const auto original=e.session().document;state.model.open=false;check(e.session().document==original,"annulla non modifica modello");
    auto cutter=run(e,"sketch.inspect",{{"sketch",second}})["regions"][0]["id"];
    auto cut=run(e,"feature.extrude",{{"sketch",second},{"profiles",J::array({cutter})},{"operation","cut"},{"target_bodies",J::array({plate["body_id"]})},{"extent","through_all"}});
    close(volume(e,plate["body_id"]),18000-60*std::acos(-1.));
    run(e,"sketch.visibility",{{"sketch",first},{"visible",nullptr}});
    check(!run(e,"sketch.inspect",{{"sketch",first}})["visible"].get<bool>(),"ripristino visibilità automatica");
    run(e,"edit.undo");check(run(e,"sketch.inspect",{{"sketch",first}})["visible"].get<bool>(),"undo visibilità");run(e,"edit.redo");
    run(e,"sketch.visibility",{{"sketch",second},{"visible",false}});close(volume(e,plate["body_id"]),18000-60*std::acos(-1.));
    auto serialized=doc::to_json(e.session().document);doc::Document restored;std::string error;check(doc::from_json(serialized,restored,error),error);check(restored==e.session().document&&restored.features.at(second).data["visible"]==false,"salvataggio riapertura con visibilità manuale");
    for(auto bad:{J{{"sketch",first},{"visible","yes"}},J{{"sketch","missing"},{"visible",true}},J{{"sketch",first}}}){const auto before=e.session().document;check(!e.execute("sketch.visibility",bad).ok,"input visibilità rifiutato");check(e.session().document==before,"errore atomico");}
    run(e,"feature.delete",{{"feature_id",first}});check(!run(e,"feature.get",{{"feature_id",plate["feature_id"]}})["error"].get<std::string>().empty(),"dipendenza eliminata segnalata");run(e,"edit.undo");
    run(e,"timeline.rollback",{{"index",1}});auto inserted=run(e,"sketch.create",{{"plane","XZ"}})["sketch"].get<std::string>();
    check(e.session().document.timeline[1]==inserted&&e.session().document.rollback==2,"nuovo schizzo inserito al marker");
    state.selected_profile_sketch=first;state.selected_profiles={"profilo-vecchio"};run(e,"sketch.rect",{{"width","10 mm"},{"height","5 mm"}});ui::begin_model(state,e,false);
    check(state.model.sketch==inserted&&state.model.profiles.size()==1&&state.model.profiles[0]!="profilo-vecchio","E non eredita selezioni di altri schizzi");
    run(e,"document.new");ui::begin_model(state,e,false);check(state.model.sketch.empty()&&state.model.profiles.empty(),"documento nuovo senza selezioni fantasma");
    std::cout<<"Workflow schizzi/estrusioni: visibilità, riuso, unione, taglio, quote, modifica, annulla, undo/redo, roundtrip, errori e marker OK\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
