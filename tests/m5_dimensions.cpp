#include "shell.hpp"
#include "automation/dispatch.hpp"
#include "ui/frame.hpp"
#include "ui/sketch_view.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <fstream>
#include <iostream>
#include <cmath>
#include <stdexcept>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
using namespace sagomacad;using J=nlohmann::json;
void check(bool b,const std::string& m){if(!b)throw std::runtime_error(m);}
int main(int argc,char** argv){try{
    check(argc>2,"scenario and evidence directory required");std::ifstream input(argv[1]);auto cases=J::parse(input).at("cases");const auto root=std::filesystem::path(argv[2]);std::filesystem::create_directories(root);
    for(bool light:{true,false})for(auto size:{std::pair{1440,900},std::pair{1280,720}}){
        engine::Engine e;Shell shell(size.first,size.second,1,true,light);check(shell.valid(),shell.error());shell.bind_engine(&e);
        auto frame=[&](){shell.render();};auto settle=[&](){frame();frame();frame();};settle();
        auto command=[&](const char* id,J args=J::object()){auto r=automation::dispatch(e,"engine.execute",{{"command",id},{"params",args}});check(r.contains("result"),r.dump());return r["result"];};
        auto mouse=[&](float x,float y,bool down){ImGui::GetIO().AddMousePosEvent(x,y);ImGui::GetIO().AddMouseButtonEvent(0,down);frame();};
        auto click=[&](float x,float y){mouse(x,y,false);mouse(x,y,true);mouse(x,y,false);};
        auto key=[&](ImGuiKey k){ImGui::GetIO().AddKeyEvent(k,true);frame();ImGui::GetIO().AddKeyEvent(k,false);frame();};
        for(const auto& scenario:cases){
            const auto kind=scenario.at("tool").get<std::string>();command("document.new");command("sketch.create",{{"plane","XY"}});settle();auto& s=shell.state();s.view_transition.advance(s.camera,2);s.camera.distance=140;settle();
            const auto inspection=shell.inspect();ui::Viewport v;for(const auto& r:inspection["shell_rects"])if(r["id"]=="viewport"){const auto pad=ui::theme(light).padding;v={r["x"].get<double>()+pad,r["y"].get<double>()+pad,r["width"].get<double>()-2*pad,r["height"].get<double>()-2*pad};}
            auto world=[&](double x,double y){return s.camera.project({x,y,0},v);};
            if(kind=="rect")key(ImGuiKey_R);else if(kind=="line")key(ImGuiKey_L);else if(kind=="circle")key(ImGuiKey_C);else s.sketch_tool.begin(kind);
            auto p=world(0,0);click(float(p.x),float(p.y));p=world(20,15);mouse(float(p.x),float(p.y),false);settle();
            if(!s.sketch_tool.started)std::cerr<<"tool="<<s.sketch_tool.kind<<" mode="<<s.sketch_mode<<" viewport="<<v.x<<","<<v.y<<","<<v.width<<","<<v.height<<" mouse="<<p.x<<","<<p.y<<" hover="<<(ImGui::GetCurrentContext()->HoveredWindow?ImGui::GetCurrentContext()->HoveredWindow->Name:"none")<<'\n';
            check(s.sketch_tool.started,"first click starts "+kind);
            const auto before=e.session().document;
            const auto fields=scenario.at("fields");
            ImGui::GetIO().AddInputCharactersUTF8(fields[0].get<std::string>().c_str());settle();
            if(kind=="arc"){key(ImGuiKey_Enter);settle();check(s.sketch_tool.arc_stage==1,"arc radius accepted");p=world(0,25);mouse(float(p.x),float(p.y),false);settle();}
            else if(fields.size()>1){key(ImGuiKey_Tab);settle();}
            if(fields.size()>1){ImGui::GetIO().AddInputCharactersUTF8(fields[1].get<std::string>().c_str());settle();}
            check(e.session().document==before,"live dimensions leave document unchanged");
            check(s.sketch_live_fields.size()==fields.size(),"one field per measure "+kind);
            if(kind=="rect"){
                auto a=s.sketch_live_fields.at("0"),b=s.sketch_live_fields.at("1");
                check(std::hypot(a[0]-b[0],a[1]-b[1])>ui::theme(light).sketch.dimension_offset,"rectangle fields are separated on their edges");
                check(s.sketch_tool.active_field==1,"Tab changes active edge");
            }
            const auto suffix=kind+"-"+(light?"light-":"dark-")+std::to_string(size.first)+".png";
            auto capture=[&](const std::string& name){settle();std::string error;auto path=(root/(name+suffix)).string();check(shell.screenshot(path,error),error);return path;};
            const auto png=capture("drawing-");
            auto m=std::find_if(s.sketch_measures.begin(),s.sketch_measures.end(),[&](const auto& m){return m.field==s.sketch_tool.active_field;});check(m!=s.sketch_measures.end(),"active measure metadata");
            // Sample the actual rendered stroke, away from the detached input.
            auto stroke=s.camera.project(ui::sketch_world("XY",m->position),v);int w=0,h=0,ch=0;auto* pixels=stbi_load(png.c_str(),&w,&h,&ch,4);check(pixels,"read evidence PNG");int accent=0;const auto c=ui::theme(light).accent;
            for(int y=int(stroke.y)-3;y<=int(stroke.y)+3;++y)for(int x=int(stroke.x)-3;x<=int(stroke.x)+3;++x)if(x>=0&&x<w&&y>=0&&y<h){auto* pixel=pixels+(y*w+x)*4;if(std::abs(pixel[0]-c.r*255)<8&&std::abs(pixel[1]-c.g*255)<8&&std::abs(pixel[2]-c.b*255)<8)++accent;}
            stbi_image_free(pixels);if(kind=="rect"||kind=="line")check(accent>0,"active measured stroke uses accent");
            key(ImGuiKey_Enter);settle();check(!s.sketch_tool.started,"Enter confirms "+kind+": "+s.sketch_error);
            auto inspected=command("sketch.inspect");check(inspected["entities"].size()==scenario["entities"].get<size_t>(),"created entities");check(inspected["dimensions"].size()==scenario["dimensions"].get<size_t>(),"typed dimensions become constraints");
            check(s.sketch_dimension_rects.size()==fields.size(),"persistent labels stay on figure");capture("created-");
            const auto selected_dimension=std::find_if(inspected["dimensions"].begin(),inspected["dimensions"].end(),[](const auto& d){return d.at("kind")!="angle";});check(selected_dimension!=inspected["dimensions"].end(),"length or radius dimension exists");
            const auto id=selected_dimension->at("id").template get<std::string>();auto r=s.sketch_dimension_rects.at(id);click(r[0]+r[2]/2,r[1]+r[3]/2);click(r[0]+r[2]/2,r[1]+r[3]/2);settle();
            check(s.sketch_dimension_open&&s.sketch_dimension_id==id,"double click edits persistent dimension "+kind);
            const auto edit=s.sketch_dimension_edit_rect;check(std::hypot(edit[0]-r[0],edit[1]-r[1])<ui::theme(light).padding*2,"editor is on top of dimension");capture("editing-");
            ImGui::GetIO().AddInputCharactersUTF8(scenario["edit"].get<std::string>().c_str());settle();key(ImGuiKey_Enter);settle();check(!s.sketch_dimension_open,"inline edit confirms");
            const auto edited=command("sketch.inspect");const auto changed=std::find_if(edited["dimensions"].begin(),edited["dimensions"].end(),[&](const auto& d){return d.at("id")==id;});check(changed!=edited["dimensions"].end()&&changed->at("expression")==scenario["edit"],"dimension edit persisted");
            auto document=doc::to_json(e.session().document);doc::Document loaded;std::string error;check(doc::from_json(document,loaded,error)&&loaded==e.session().document,"dimension roundtrip");
        }
    }
    std::cout<<"16 M5 dimension scenarios: separate fields, accent pixels, Tab/Enter, constraints, inline edit and persistence OK\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
