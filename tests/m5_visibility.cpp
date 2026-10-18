#include "shell.hpp"
#include "automation/dispatch.hpp"
#include "ui/model_editor.hpp"
#include "ui/sketch_view.hpp"
#include <imgui.h>
#include <cstdio>
#include <iostream>
#include <stdexcept>
using namespace sagomacad;using J=nlohmann::json;
void check(bool b,const std::string& m){if(!b)throw std::runtime_error(m);}
J run(engine::Engine& e,const char* id,J p=J::object()){auto r=e.execute(id,p);check(r.ok,std::string(id)+": "+r.error);return r.value;}
int main(int argc,char** argv){try{
    engine::Engine e;check(run(e,"preferences.get")["hide_sketch_after_feature"]==false,"default visible");
    auto sk=run(e,"sketch.create",{{"plane","XY"}})["sketch"];
    auto rect=run(e,"sketch.rect",{{"width","20 mm"},{"height","10 mm"}});
    auto circle=run(e,"sketch.circle",{{"x",40},{"y",5},{"diameter","6 mm"}});
    run(e,"sketch.finish");
    run(e,"feature.extrude",{{"sketch",sk},{"profiles",J::array({rect["entities"]})},{"distance","5 mm"}});
    auto inspected=run(e,"sketch.inspect",{{"sketch",sk}});check(inspected["visible"]==true&&inspected["regions"].size()==2,"unused circle stays visible and available");
    {
        Shell view(1440,900,1,true,true);check(view.valid(),view.error());view.bind_engine(&e);view.render();view.render();
        view.state().camera=ui::sketch_camera(view.state().camera,"XY");view.state().camera.distance=140;view.render();view.render();
        const auto inspection=view.inspect();ui::Viewport viewport;for(const auto& r:inspection["shell_rects"])if(r["id"]=="viewport"){auto pad=ui::light.padding;viewport={r["x"].get<double>()+pad,r["y"].get<double>()+pad,r["width"].get<double>()-2*pad,r["height"].get<double>()-2*pad};}
        auto point=view.state().camera.project({40,5,0},viewport);
        auto mouse=[&](float x,float y,bool down){ImGui::GetIO().AddMousePosEvent(x,y);ImGui::GetIO().AddMouseButtonEvent(0,down);view.render();};
        mouse(float(point.x),float(point.y),false);mouse(float(point.x),float(point.y),true);mouse(float(point.x),float(point.y),false);
        check(view.state().selected_profile_sketch==sk.get<std::string>()&&view.state().selected_profiles.size()==1,"unused circle selectable by actual mouse click");
        ImGui::GetIO().AddKeyEvent(ImGuiKey_E,true);view.render();ImGui::GetIO().AddKeyEvent(ImGuiKey_E,false);view.render();view.render();
        auto& model=view.state().model;check(model.open&&model.profiles.size()==1&&model.operation==3,"E opens new extrusion of detached unused circle");
        std::snprintf(model.distance.data(),model.distance.size(),"8 mm");view.render();view.render();auto ok=model.fields.at("ok");
        mouse(ok[0]+ok[2]/2,ok[1]+ok[3]/2,false);mouse(ok[0]+ok[2]/2,ok[1]+ok[3]/2,true);mouse(ok[0]+ok[2]/2,ok[1]+ok[3]/2,false);view.render();check(!model.open,"OK extrudes unused circle");
    }
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
    if(argc>1)for(bool light:{true,false})for(auto size:{std::pair{1440,900},std::pair{1280,720}}){
        engine::Engine fresh;Shell shell(size.first,size.second,1,true,light);shell.bind_engine(&fresh);shell.state().preferences_open=true;for(int i=0;i<3;++i)shell.render();
        const auto root=std::filesystem::path(argv[1]);std::filesystem::create_directories(root);std::string error;
        check(shell.screenshot((root/(std::string("preferences-")+(light?"light-":"dark-")+std::to_string(size.first)+".png")).string(),error),error);
    }
}catch(const std::exception& e){std::cerr<<e.what();return 1;}}
