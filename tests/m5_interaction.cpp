#include "shell.hpp"
#include "ui/surface_editor.hpp"
#include <imgui.h>
#include <filesystem>
#include <thread>
#include <chrono>
#include <numbers>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <stdexcept>
using namespace sagomacad;using J=nlohmann::json;
void check(bool b,const std::string& m){if(!b)throw std::runtime_error(m);}
J run(engine::Engine& e,const char* id,J p=J::object()){auto r=e.execute(id,p);check(r.ok,std::string(id)+": "+r.error);return r.value;}
int main(int argc,char** argv){try{
    auto root=std::filesystem::path(argc>1?argv[1]:".");std::filesystem::create_directories(root);
    for(bool light:{true,false})for(auto size:{std::pair{1440,900},std::pair{1280,720}}){
        engine::Engine e;auto base=[&](){run(e,"document.new");run(e,"primitive.add",{{"kind","box"},{"dimensions",{{"width","40 mm"},{"depth","40 mm"},{"height","20 mm"}}}});};base();
        Shell shell(size.first,size.second,1,true,light);check(shell.valid(),shell.error());shell.bind_engine(&e);shell.render();shell.render();
        auto frame=[&](){shell.render();shell.render();};
        auto mouse=[&](float x,float y,bool down){auto& io=ImGui::GetIO();io.AddMousePosEvent(x,y);io.AddMouseButtonEvent(0,down);shell.render();};
        auto click=[&](float x,float y){mouse(x,y,false);mouse(x,y,true);mouse(x,y,false);frame();};
        auto key=[&](ImGuiKey k){ImGui::GetIO().AddKeyEvent(k,true);shell.render();ImGui::GetIO().AddKeyEvent(k,false);frame();};
        auto viewport=[&](){const auto inspection=shell.inspect();for(const auto& r:inspection["shell_rects"])if(r["id"]=="viewport"){double pad=ui::theme(light).padding;return ui::Viewport{r["x"].get<double>()+pad,r["y"].get<double>()+pad,r["width"].get<double>()-2*pad,r["height"].get<double>()-2*pad};}throw std::runtime_error("viewport absent");};
        auto project=[&](kernel::Vec3 point){return shell.state().camera.project(point,viewport());};
        auto world_click=[&](kernel::Vec3 p){auto q=project(p);click(float(q.x),float(q.y));};
        std::string error,suffix=std::string(light?"light-":"dark-")+std::to_string(size.first)+".png";
        auto capture=[&](const std::string& name){if(name.rfind("11-",0)!=0)ImGui::GetIO().AddMousePosEvent(-1000,-1000);frame();check(shell.screenshot((root/(name+suffix)).string(),error),error);};
        auto top=[&](){return run(e,"geometry.query",{{"kind","face"},{"normal",{0,0,1}}})["entities"][0]["ref"];};
        auto vertical=[&](){J refs=J::array();auto queried=run(e,"geometry.query",{{"kind","edge"},{"parallel_to",{0,0,1}}});for(const auto& row:queried["entities"])refs.push_back(row["ref"]);return refs;};
        auto p=project({20,20,20});mouse(float(p.x),float(p.y),false);frame();check(shell.state().hovered_entity&&shell.state().hovered_entity->kind==engine::PickKind::Face,"face hover");capture("11-faccia-");
        p=project({40,0,10});mouse(float(p.x),float(p.y),false);frame();check(shell.state().hovered_entity&&shell.state().hovered_entity->kind==engine::PickKind::Edge,"edge hover");capture("11-spigoli-");world_click({40,0,10});check(run(e,"selection.get")["refs"].size()==1,"click selects edge");ImGui::GetIO().AddKeyEvent(ImGuiMod_Ctrl,true);world_click({0,0,10});ImGui::GetIO().AddKeyEvent(ImGuiMod_Ctrl,false);frame();check(run(e,"selection.get")["refs"].size()==2,"Ctrl adds second edge: "+run(e,"selection.get").dump());
        run(e,"selection.set",{{"refs",J::array()}});auto overlap=run(e,"primitive.add",{{"kind","box"},{"position",{{"x","30 mm"},{"y","-5 mm"}}},{"dimensions",{{"width","15 mm"},{"depth","15 mm"},{"height","20 mm"}}}});frame();p=project({35,2,15});mouse(float(p.x),float(p.y),true);std::this_thread::sleep_for(std::chrono::milliseconds(520));shell.render();capture("11-seleziona-altro-");mouse(float(p.x),float(p.y),false);key(ImGuiKey_Escape);check(!ImGui::IsPopupOpen("",ImGuiPopupFlags_AnyPopupId|ImGuiPopupFlags_AnyPopupLevel),"Select Other closes with Escape");
        run(e,"feature.delete",{{"feature_id",overlap["feature_id"]}});frame();run(e,"selection.set",{{"refs",J::array({top()})}});ui::LayoutEngine probe;auto layout=probe.compute(float(size.first),float(size.second),1,shell.state(),ui::theme(light));auto button=layout.groups[0].buttons[0];click(button.x+button.width/2,button.y+button.height/2);check(shell.state().sketch_mode,"Create Sketch on selected face; active="+e.session().active_sketch+" status="+shell.state().status_message+" selection="+run(e,"selection.get").dump()+" button="+std::to_string(button.x)+","+std::to_string(button.y));shell.state().view_transition.advance(shell.state().camera,2);frame();check(std::abs(shell.state().camera.forward().z+1)<1e-6,"camera aligns to support");
        run(e,"sketch.circle",{{"x",10},{"y",10},{"diameter","10 mm"}});capture("12-schizzo-su-faccia-");run(e,"sketch.finish");frame();run(e,"selection.set",{{"refs",J::array({top()})}});key(ImGuiKey_E);check(shell.state().model.open&&shell.state().model.face.is_object(),"E accepts selected face");check(shell.state().model.preview_error.empty(),shell.state().model.preview_error);shell.state().view_transition.advance(shell.state().camera,2);frame();capture("13-estrudi-faccia-");key(ImGuiKey_Escape);
        run(e,"selection.set",{{"refs",vertical()}});key(ImGuiKey_F);auto& m=shell.state().surface;check(m.open&&m.command=="feature.fillet"&&m.edges.size()==4,"F with four edges");frame();check(m.preview_error.empty(),m.preview_error);auto original=e.session().document;float hx=m.handle_x,hy=m.handle_y;mouse(hx,hy,false);mouse(hx,hy,true);mouse(hx+20,hy-15,true);mouse(hx+20,hy-15,false);frame();check(e.session().document==original,"fillet preview leaves document untouched");check(std::abs(std::stod(m.value.data())-3)>.01,"radius handle changes value");std::snprintf(m.value.data(),m.value.size(),"3 mm");frame();capture("14-raccordo-");check(m.fields.contains("ok"),"fillet OK field");auto ok=m.fields.at("ok");click(ok[0]+ok[2]/2,ok[1]+ok[3]/2);check(!m.open,"fillet confirmed");check(std::abs(run(e,"model.mass_properties")["volume_mm3"].get<double>()-(32000-(4-std::numbers::pi)*9*20))<1e-4,"UI fillet volume");
        base();frame();run(e,"selection.set",{{"refs",vertical()}});ui::begin_surface(shell.state(),e,"feature.chamfer");frame();check(m.preview_error.empty(),m.preview_error);capture("15-smusso-");m.open=false;
        base();frame();key(ImGuiKey_H);check(m.open&&m.command=="feature.hole","H opens Hole");world_click({10,10,20});m.through=true;frame();auto before_hole=e.session().document;auto dimension=m.fields.at("dimension0");float dx=dimension[0]+dimension[2]/2,dy=dimension[1]+dimension[3]/2;mouse(dx,dy,false);mouse(dx,dy,true);mouse(dx+20,dy,true);mouse(dx+20,dy,false);frame();check(std::abs(std::stod(m.offsets[0].data())-10)>.01&&e.session().document==before_hole,"position dimension drag is a preview");for(int i=0;i<2;++i){auto r=m.fields.at("offset"+std::to_string(i));click(r[0]+r[2]/2,r[1]+r[3]/2);ImGui::GetIO().AddInputCharactersUTF8("10 mm");shell.render();key(ImGuiKey_Enter);check(std::string(m.offsets[i].data())=="10 mm","position dimension editable");}frame();check(m.references.size()==2&&m.preview_error.empty(),"one click places a dimensioned hole: "+m.preview_error);capture("16-foro-");check(e.session().document.timeline.size()==1,"hole remains preview");ok=m.fields.at("ok");click(ok[0]+ok[2]/2,ok[1]+ok[3]/2);check(!m.open,"hole confirmed");double volume=run(e,"model.mass_properties")["volume_mm3"];check(std::abs(volume-(32000-320*std::numbers::pi))<1e-4,"through hole volume");
        shell.state().show_topology_labels=true;frame();check(!shell.state().topology_labels.empty(),"stable labels drawn");capture("agente-etichette-");
    }
    std::cout<<"M5 UI: face/edge hover, Ctrl selection, hold menu, face sketch/camera, E/F/H, preview/confirm and labels in two themes and sizes OK\n";
}catch(const std::exception& e){std::cerr<<e.what();return 1;}}
