#include "engine/tools/sketch_tool.hpp"
#include "solver/sketch_solver.hpp"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
using namespace sagomacad;
void check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
int main(){try{
    engine::Engine e;auto run=[&](const char* id,nlohmann::json args=nlohmann::json::object()){auto r=e.execute(id,args);if(!r.ok)throw std::runtime_error(std::string(id)+": "+r.error);return r.value;};
    auto id=run("sketch.create",{{"plane","XY"}})["sketch"];
    engine::SketchTool tool;tool.begin("rect");tool.click(0,0);tool.move(30,15);tool.text("40");tool.tab();tool.text("20");auto r=tool.confirm(e);check(r.ok,"40 Tab20 Invio");
    auto inspection=run("sketch.inspect");check(inspection["state"]=="constrained"&&inspection["degrees_of_freedom"]==0,"rettangolo nero");
    check(inspection["profiles"].size()==1,"profilo chiuso");
    run("param.set",{{"name","foro"},{"expression","10 mm"}});run("sketch.circle",{{"diameter","foro"}});
    run("param.set",{{"name","foro"},{"expression","18 mm"}});inspection=run("sketch.inspect");check(std::abs(inspection["entities"].back()["radius"].get<double>()-9)<1e-5,"cerchio segue parametro");
    check(!e.execute("param.delete",{{"name","foro"}}).ok,"parametro in uso protetto");
    run("param.set",{{"name","diametro"},{"old_name","foro"},{"expression","12 mm"}});inspection=run("sketch.inspect");check(inspection["dimensions"].back()["expression"]=="diametro","rinomina quota");
    const auto path=std::filesystem::temp_directory_path()/"sagomacad-sketch-test.sagomacad";
    auto before=e.session().document;run("document.save",{{"path",path.string()}});run("document.new");run("document.open",{{"path",path.string()}});check(before==e.session().document,"roundtrip schizzo timeline");std::filesystem::remove(path);
    run("sketch.edit",{{"sketch",id}});auto dim=run("sketch.inspect")["dimensions"][0]["id"];
    run("sketch.dimension.set",{{"dimension",dim},{"expression","60 mm"}});auto changed=run("sketch.inspect");check(std::abs(changed["points"][1]["x"].get<double>()-60)<1e-5,"modifica quota");
    run("edit.undo");check(std::abs(run("sketch.inspect")["points"][1]["x"].get<double>()-40)<1e-5,"undo quota");run("edit.redo");
    auto line=changed["entities"][0]["id"];auto conflict=run("sketch.dimension.add",{{"kind","length"},{"refs",{line}},{"expression","90 mm"}});check(conflict["solver"]["state"]=="conflict"&&!conflict["solver"]["error"].get<std::string>().empty(),"conflitto senza crash");
    for(const char* plane:{"XZ","YZ"}){run("sketch.create",{{"plane",plane}});run("sketch.arc",{{"radius","10 mm"},{"start_angle","0 deg"},{"end_angle","90 deg"}});check(run("sketch.inspect")["plane"]==plane,"piano origine");}
    run("sketch.finish");check(e.session().active_sketch.empty(),"fine schizzo");
    engine::Engine batch;auto combined=batch.execute_batch(nlohmann::json::array({
        {{"command","sketch.create"},{"params",{{"plane","XY"}}}},
        {{"command","sketch.rect"},{"params",{{"width","60 mm"},{"height","40 mm"},{"centered",true}}}},
        {{"command","sketch.circle"},{"params",{{"diameter","10 mm"}}}}
    }));check(combined.ok&&batch.session().history.size()==1,"batch schizzo un solo undo");
    check(!batch.session().active_sketch.empty()&&batch.execute("sketch.inspect").value["state"]=="constrained","batch mantiene schizzo attivo");
    check(batch.execute("edit.undo").ok&&batch.session().document.features.empty(),"undo batch geometria");
    engine::Engine negative;check(negative.execute("sketch.create",{{"plane","XY"}}).ok,"piano negativo");
    engine::SketchTool backwards;backwards.begin("rect");backwards.click(0,0);backwards.move(-40,-20);backwards.text("40");backwards.tab();backwards.text("20");
    auto flipped=backwards.confirm(negative);check(flipped.ok&&flipped.value["solver"]["state"]=="constrained","rettangolo verso sinistra e in basso");
    check(std::abs(flipped.value["solver"]["points"][2]["x"].get<double>()+40)<1e-5,"direzione rettangolo");
    run("param.set",{{"name","angolo"},{"expression","90 deg"}});
    run("sketch.create",{{"plane","XY"}});run("sketch.arc",{{"radius","10 mm"},{"start_angle","0 deg"},{"end_angle","angolo"},{"anchor",true}});
    run("param.set",{{"name","angolo"},{"expression","60 deg"}});auto arc=run("sketch.inspect");
    check(arc["state"]=="constrained"&&std::abs(arc["points"][2]["x"].get<double>()-5)<1e-5,"arco conserva espressioni angolari");
    std::cout<<"Sketch commands, live tool, parameters, persistence and undo OK\n";
}catch(const std::exception& ex){std::cerr<<ex.what()<<'\n';return 1;}}
