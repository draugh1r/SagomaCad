#include "shell.hpp"
#include "ui/model_editor.hpp"
#include <imgui.h>
#include <filesystem>
#include <numbers>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
using namespace sagomacad;using J=nlohmann::json;
void check(bool b,const std::string& m){if(!b)throw std::runtime_error(m);}
J run(engine::Engine& e,const char* id,J p=J::object()){auto r=e.execute(id,p);check(r.ok,std::string(id)+": "+r.error);return r.value;}
int main(int argc,char** argv){try{
    const auto root=std::filesystem::path(argc>1?argv[1]:".");std::filesystem::create_directories(root);
    for(bool light:{true,false})for(auto size:{std::pair{1440,900},std::pair{1280,720}}){
        engine::Engine e;auto sk=run(e,"sketch.create",{{"plane","XY"}})["sketch"];run(e,"sketch.rect",{{"width","60 mm"},{"height","40 mm"},{"centered",true}});run(e,"sketch.circle",{{"diameter","20 mm"}});run(e,"sketch.finish");
        Shell shell(size.first,size.second,1,true,light);check(shell.valid(),shell.error());shell.bind_engine(&e);shell.state().camera.home();shell.state().camera.distance=180;shell.render();shell.render();
        auto mouse=[&](float x,float y,bool down){auto& io=ImGui::GetIO();io.AddMousePosEvent(x,y);io.AddMouseButtonEvent(0,down);shell.render();};
        auto click=[&](float x,float y){mouse(x,y,false);mouse(x,y,true);mouse(x,y,false);shell.render();};
        auto field=[&](const char* key){check(shell.state().model.fields.contains(key),std::string("Campo assente: ")+key);auto r=shell.state().model.fields.at(key);click(r[0]+r[2]/2,r[1]+r[3]/2);};
        auto key=[&](ImGuiKey k){ImGui::GetIO().AddKeyEvent(k,true);shell.render();ImGui::GetIO().AddKeyEvent(k,false);shell.render();};
        auto viewport=[&](){const auto inspection=shell.inspect();for(const auto& r:inspection["shell_rects"])if(r["id"]=="viewport"){const auto& t=ui::theme(light);double pad=t.padding;return ui::Viewport{r["x"].get<double>()+pad,r["y"].get<double>()+pad,r["width"].get<double>()-2*pad,r["height"].get<double>()-2*pad};}throw std::runtime_error("viewport assente");};
        auto world_click=[&](kernel::Vec3 point){auto p=shell.state().camera.project(point,viewport());click(float(p.x),float(p.y));};
        std::string error;auto suffix=std::string(light?"light-":"dark-")+std::to_string(size.first)+".png";
        auto capture=[&](const std::string& name){shell.render();check(shell.screenshot((root/(name+suffix)).string(),error),error);};
        const auto original=e.session().document;key(ImGuiKey_E);check(shell.state().model.open&&!shell.state().model.revolve,"E apre Estrudi");world_click({-20,0,0});
        check(shell.state().model.profiles.size()==1,"selezione della sola corona");check(shell.state().model.preview_error.empty(),shell.state().model.preview_error);check(e.session().document==original,"anteprima non modifica il documento");
        world_click({6,-6,0});check(shell.state().model.profiles.size()==2,"disco selezionabile separatamente");world_click({6,-6,0});check(shell.state().model.profiles.size()==1,"rimozione del solo disco");capture("07-estrudi-");
        field("operation");check(shell.state().model.operation_menu,"menu Operazione aperto");capture("08-operazione-");
        check(shell.state().model.fields.contains("op3"),"quattro operazioni con hitbox");field("op3");
        auto& m=shell.state().model;float hx=m.handle_x,hy=m.handle_y;mouse(hx,hy,false);mouse(hx,hy,true);mouse(hx,hy-30,true);mouse(hx,hy-30,false);shell.render();
        check(std::abs(std::stod(m.distance.data())-10)>.1,"freccia aggiorna la distanza");check(e.session().document==original,"drag è solo anteprima");click(m.value_x+30,m.value_y+10);ImGui::GetIO().AddInputCharactersUTF8("10 mm");shell.render();key(ImGuiKey_Enter);check(std::string(m.distance.data())=="10 mm","casella nel viewport modificabile");field("ok");check(!m.open,"OK chiude Estrudi");
        double expected=(2400-100*std::numbers::pi)*10;check(std::abs(run(e,"model.mass_properties")["volume_mm3"].get<double>()-expected)<1e-4,"volume della corona UI");
        auto feature=e.session().document.timeline[1];const auto item=shell.state().timeline_items.at(1);click(item.x+item.width/2,item.y+item.height/2);click(item.x+item.width/2,item.y+item.height/2);check(m.open&&m.feature_id==feature,"doppio clic riapre il dialogo");field("cancel");
        const auto source=sk.get<std::string>();check(!shell.state().sketch_browser.at(source).visible,"occhio indica sorgente nascosta dopo estrusione");
        auto eye=shell.state().sketch_browser.at(source).eye;click(eye[0]+eye[2]/2,eye[1]+eye[3]/2);shell.render();check(shell.state().sketch_browser.at(source).visible,"occhio mostra di nuovo lo schizzo");
        world_click({-20,0,0});key(ImGuiKey_E);check(m.sketch==source&&m.profiles.size()==1&&m.preview_error.empty(),"profilo consumato di nuovo selezionabile");capture("audit-riuso-profilo-");
        const auto before_reuse=e.session().document;field("operation");field("op0");std::snprintf(m.distance.data(),m.distance.size(),"15 mm");shell.render();field("ok");
        check(!m.open,"seconda estrusione dello stesso profilo confermata");check(std::abs(run(e,"model.mass_properties")["volume_mm3"].get<double>()-expected*1.5)<1e-4,"seconda estrusione Unisci sullo stesso solido");
        run(e,"edit.undo");check(e.session().document==before_reuse,"undo seconda estrusione ripristina modello");run(e,"sketch.visibility",{{"sketch",source},{"visible",nullptr}});shell.render();
        const auto row=shell.state().sketch_browser.at(source).row;click(row[0]+row[2]/2,row[1]+row[3]/2);key(ImGuiKey_E);
        check(m.sketch==source&&m.profiles.empty(),"browser permette riuso schizzo nascosto con più regioni");world_click({-20,0,0});check(m.profiles.size()==1&&m.preview_error.empty(),"selezione regione da schizzo nascosto nel dialogo");
        field("cancel");check(!shell.state().sketch_browser.at(source).visible&&shell.state().selected_profile_sketch.empty(),"annulla ripristina nascondimento senza selezioni fantasma");
        // A second sketch provides a real red cut preview and target body field.
        run(e,"primitive.add",{{"kind","box"},{"position",{{"x","-60 mm"},{"y","-10 mm"}}},{"dimensions",{{"width","20 mm"},{"depth","20 mm"},{"height","10 mm"}}}});
        run(e,"sketch.create",{{"plane","XY"}});run(e,"sketch.circle",{{"x",20},{"diameter","5 mm"}});run(e,"sketch.finish");shell.render();key(ImGuiKey_E);world_click({20,0,0});field("operation");field("op1");shell.render();check(m.operation==1&&m.targets.size()==2,"Taglia e due corpi di destinazione");check(m.preview_error.empty(),m.preview_error);capture("08-taglia-");
        // The opaque planar cap must stay uniformly shaded around the hole:
        // rear triangles used to paint dark wedges across these visible points.
        int image_width=0,image_height=0,channels=0;auto path=(root/("08-taglia-"+suffix)).string();
        auto* pixels=stbi_load(path.c_str(),&image_width,&image_height,&channels,4);check(pixels!=nullptr,"lettura screenshot");
        std::array<int,3> reference{};bool first=true;
        for(double x:{-25.,-15.,15.,25.})for(double y:{-15.,15.}){auto projected=shell.state().camera.project({x,y,10},viewport());int px=int(projected.x),py=int(projected.y);check(px>=0&&py>=0&&px<image_width&&py<image_height,"punto nella cattura");auto pixel=pixels+(py*image_width+px)*4;
            for(int channel=0;channel<3;++channel){if(first)reference[channel]=pixel[channel];else check(std::abs(int(pixel[channel])-reference[channel])<=2,"faccia piana senza triangoli posteriori sovrapposti");}first=false;}
        stbi_image_free(pixels);field("ok");
        check(std::abs(run(e,"model.mass_properties")["volume_mm3"].get<double>()-(expected-62.5*std::numbers::pi))<1e-4,"taglio UI");
        // Start Revolve through the third command in the Create ribbon group.
        run(e,"document.new");sk=run(e,"sketch.create",{{"plane","XZ"}})["sketch"];run(e,"sketch.rect",{{"x",5},{"width","30 mm"},{"height","20 mm"}});run(e,"sketch.finish");shell.render();shell.state().camera.home();shell.state().camera.target={0,0,10};shell.state().camera.distance=180;shell.render();
        ui::LayoutEngine probe;const auto ribbon=probe.compute(float(size.first),float(size.second),1,shell.state(),ui::theme(light));const auto command=ribbon.groups[0].buttons[2];click(command.x+command.width/2,command.y+command.height/2);
        check(m.open&&m.revolve,"Rivoluzione dal gruppo Crea");world_click({20,0,10});field("axis");check(m.pick_axis,"campo Asse attiva selezione");world_click({0,0,50});shell.render();check(m.axis=="Z","selezione asse Z nel viewport");check(m.preview_error.empty(),m.preview_error);capture("09-rivoluzione-");
        hx=m.handle_x;hy=m.handle_y;mouse(hx,hy,false);mouse(hx,hy,true);check(m.dragging,"maniglia angolo attiva");mouse(hx-m.drag_px*90,hy-m.drag_py*90,true);mouse(hx-m.drag_px*90,hy-m.drag_py*90,false);shell.render();
        double angle=std::stod(m.angle.data());check(std::abs(angle-270)<1,"maniglia aggiorna l'angolo");field("ok");check(!m.open,"OK rivoluzione");double measured=run(e,"model.mass_properties")["volume_mm3"].get<double>();check(std::abs(measured-24000*std::numbers::pi*angle/360)<1e-3,"volume rivoluzione UI: "+std::to_string(measured)+", angolo "+std::to_string(angle));
        run(e,"document.new");auto older=run(e,"sketch.create",{{"plane","XY"}})["sketch"].get<std::string>();run(e,"sketch.rect",{{"width","30 mm"},{"height","20 mm"}});
        auto newer=run(e,"sketch.create",{{"plane","XY"}})["sketch"].get<std::string>();run(e,"sketch.rect",{{"width","40 mm"},{"height","20 mm"}});run(e,"sketch.finish");shell.render();
        key(ImGuiKey_E);auto choose=shell.state().sketch_browser.at(newer).row;click(choose[0]+choose[2]/2,choose[1]+choose[3]/2);world_click({5,5,0});
        check(m.sketch==newer&&m.profiles.size()==1,"browser disambigua profili sovrapposti nello stesso piano");capture("audit-profili-sovrapposti-");key(ImGuiKey_Escape);check(!m.open&&shell.state().selected_profiles.empty()&&shell.state().selected_profile_sketch.empty(),"Esc pulisce dialogo e preselezione");
        run(e,"sketch.edit",{{"sketch",older}});shell.render();key(ImGuiKey_E);check(m.open&&m.sketch==older&&m.profiles.size()==1,"E dal solo profilo attivo senza selezione precedente");
        run(e,"sketch.edit",{{"sketch",newer}});shell.render();check(shell.state().sketch_mode&&!m.open,"entrare in uno schizzo chiude il precedente dialogo Estrudi");
    }
    std::cout<<"UI M4: selezione corona, E, drag distanza, menu, taglio, Crea/Rivoluzione, asse, drag angolo e doppio clic OK\n";
}catch(const std::exception& e){std::cerr<<e.what();return 1;}}
