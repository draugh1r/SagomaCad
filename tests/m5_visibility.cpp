#include "shell.hpp"
#include "automation/dispatch.hpp"
#include <iostream>
#include <stdexcept>
using namespace sagomacad;using J=nlohmann::json;
void check(bool b,const std::string& m){if(!b)throw std::runtime_error(m);}
J run(engine::Engine& e,const char* id,J p=J::object()){auto r=e.execute(id,p);check(r.ok,std::string(id)+": "+r.error);return r.value;}
int main(){try{
    engine::Engine e;check(run(e,"preferences.get")["hide_sketch_after_feature"]==false,"default visible");
    auto sk=run(e,"sketch.create",{{"plane","XY"}})["sketch"];
    auto rect=run(e,"sketch.rect",{{"width","20 mm"},{"height","10 mm"}});
    auto circle=run(e,"sketch.circle",{{"x",40},{"y",5},{"diameter","6 mm"}});
    run(e,"sketch.finish");
    run(e,"feature.extrude",{{"sketch",sk},{"profiles",J::array({rect["entities"]})},{"distance","5 mm"}});
    auto inspected=run(e,"sketch.inspect",{{"sketch",sk}});check(inspected["visible"]==true&&inspected["regions"].size()==2,"unused circle stays visible and available");
    run(e,"feature.extrude",{{"sketch",sk},{"profiles",J::array({J::array({circle["entity"]})})},{"distance","8 mm"}});
    check(e.session().document.bodies.size()==2,"unused profile extrudes independently");
    run(e,"edit.undo");check(run(e,"sketch.inspect",{{"sketch",sk}})["visible"]==true,"undo keeps sketch visible");run(e,"edit.redo");
    run(e,"preferences.set",{{"hide_sketch_after_feature",true}});
    run(e,"feature.extrude",{{"sketch",sk},{"profiles",J::array({J::array({circle["entity"]})})},{"distance","2 mm"},{"operation","new"}});
    check(run(e,"sketch.inspect",{{"sketch",sk}})["visible"]==false,"enabled preference hides source");
    run(e,"edit.undo");check(run(e,"sketch.inspect",{{"sketch",sk}})["visible"]==true,"hiding shares feature undo");
    check(automation::dispatch(e,"engine.execute",{{"command","preferences.get"}})["result"]["hide_sketch_after_feature"]==true,"control preference");
    check(!e.execute("preferences.set",{{"hide_sketch_after_feature","yes"}}).ok,"strict boolean");
    const auto path=std::filesystem::temp_directory_path()/"sagomacad-m5-preferences.json";
    {Shell shell(1280,720,1,true,true);check(shell.valid(),"shell");shell.bind_engine(&e);shell.set_settings_file(path.string());run(e,"preferences.set",{{"hide_sketch_after_feature",true}});shell.render();}
    {engine::Engine fresh;Shell shell(1280,720,1,true,true);shell.set_settings_file(path.string());shell.bind_engine(&fresh);check(run(fresh,"preferences.get")["hide_sketch_after_feature"]==true,"restart persistence");}
    std::filesystem::remove(path);std::cout<<"M5 visibility, profile reuse, control, undo and preference persistence OK\n";
}catch(const std::exception& e){std::cerr<<e.what();return 1;}}
