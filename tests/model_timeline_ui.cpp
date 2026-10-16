#include "shell.hpp"
#include <imgui.h>
#include <filesystem>
#include <iostream>
#include <stdexcept>
using namespace sagomacad;using J=nlohmann::json;
void check(bool b,const std::string& m){if(!b)throw std::runtime_error(m);}
J run(engine::Engine& e,const char* id,J p=J::object()){auto r=e.execute(id,p);check(r.ok,r.error);return r.value;}
int main(int argc,char** argv){try{
    const auto root=std::filesystem::path(argc>1?argv[1]:".");std::filesystem::create_directories(root);
    for(bool light:{true,false})for(auto size:{std::pair{1440,900},std::pair{1280,720}}){
        engine::Engine e;auto sk=run(e,"sketch.create",{{"plane","XY"}})["sketch"];auto r=run(e,"sketch.rect",{{"width","50 mm"},{"height","50 mm"}});auto first=run(e,"feature.extrude",{{"sketch",sk},{"profiles",J::array({r["entities"]})},{"distance","10 mm"}})["feature_id"];
        auto vertical=run(e,"sketch.create",{{"plane","XZ"}})["sketch"];r=run(e,"sketch.rect",{{"x",5},{"width","10 mm"},{"height","20 mm"}});run(e,"feature.revolve",{{"sketch",vertical},{"profiles",J::array({r["entities"]})},{"axis","Z"},{"type","full"}});
        Shell shell(size.first,size.second,1,true,light);check(shell.valid(),shell.error());shell.bind_engine(&e);shell.render();shell.render();
        auto mouse=[&](float x,float y,bool down){auto& io=ImGui::GetIO();io.AddMousePosEvent(x,y);io.AddMouseButtonEvent(0,down);shell.render();};
        const auto items=shell.state().timeline_items;const float step=items[1].x-items[0].x;
        float mx=shell.state().timeline_marker_x,my=shell.state().timeline_marker_y+10;
        const auto history=e.session().history.size();mouse(mx,my,false);mouse(mx,my,true);mouse(mx-2*step,my,true);mouse(mx-2*step,my,false);
        check(e.session().document.rollback==2,"drag del marker fra le icone");check(e.session().history.size()==history+1,"un drag un undo");
        auto scene=regen::Regenerator{}.evaluate(e.session().document);check(scene.features.size()==2&&scene.bodies[0].exact&&!scene.bodies[1].exact,"solo il solido prima del marker");
        std::string error;auto suffix=std::string(light?"light-":"dark-")+std::to_string(size.first)+".png";
        check(shell.screenshot((root/("10-timeline-"+suffix)).string(),error),error);
        // Return to the end, then delete the first source and expose the error tooltip.
        mouse(shell.state().timeline_marker_x,my,false);mouse(shell.state().timeline_marker_x,my,true);mouse(mx,my,true);mouse(mx,my,false);check(e.session().document.rollback==4,"avanzamento marker");
        run(e,"feature.delete",{{"feature_id",sk}});shell.render();
        auto bad=run(e,"feature.get",{{"feature_id",first}});check(!bad["error"].get<std::string>().empty(),"feature in errore");
        const auto error_item=shell.state().timeline_items.at(0);mouse(error_item.x+10,error_item.y+10,false);shell.render();
        check(shell.screenshot((root/("timeline-error-"+suffix)).string(),error),error);
        auto next=regen::Regenerator{}.evaluate(e.session().document);check(next.bodies[1].exact.has_value(),"rivoluzione dopo errore applicata");
    }
    std::cout<<"UI timeline: drag, rollback effettivo, coalescenza, errore e quattro screenshot OK\n";
}catch(const std::exception& e){std::cerr<<e.what();return 1;}}
