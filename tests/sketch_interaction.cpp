#include "shell.hpp"
#include "ui/sketch_view.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <filesystem>
#include <chrono>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
using namespace sagomacad;
void check(bool b,const char* message){if(!b)throw std::runtime_error(message);}
int main(int argc,char** argv){try{
    int width=1440,height=900;bool light=true;
    if(argc>2){std::string size=argv[2];auto x=size.find('x');width=std::stoi(size.substr(0,x));height=std::stoi(size.substr(x+1));}
    if(argc>3)light=std::string(argv[3])!="dark";
    engine::Engine e;Shell shell(width,height,1,true,light);check(shell.valid(),"shell");shell.bind_engine(&e);
    auto frame=[&](){shell.render();};for(int i=0;i<3;++i)frame();
    auto* window=ImGui::FindWindowByName("Viewport");check(window,"viewport");
    ui::Viewport v{window->WorkRect.Min.x,window->WorkRect.Min.y,window->WorkRect.GetWidth(),window->WorkRect.GetHeight()};
    auto mouse=[&](double x,double y,bool pressed){ImGui::GetIO().AddMousePosEvent(float(x),float(y));ImGui::GetIO().AddMouseButtonEvent(0,pressed);frame();};
    auto key=[&](ImGuiKey k){ImGui::GetIO().AddKeyEvent(k,true);frame();ImGui::GetIO().AddKeyEvent(k,false);frame();};
    auto snapshot=[&](const char* name){
        const bool verify=std::string(name)=="m3-sketch-mode.png";
        if(argc<=1&&!verify)return;
        frame();frame();std::string error;
        auto path=argc>1?std::filesystem::path(argv[1])/name:std::filesystem::temp_directory_path()/("sagomacad-m3-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".png");
        check(shell.screenshot(path.string(),error),error.c_str());
        if(verify){int w=0,h=0,channels=0;auto* pixels=stbi_load(path.string().c_str(),&w,&h,&channels,4);check(pixels&&w==width&&h==height,"PNG leggibile");
            const auto c=ui::theme(light).muted;int matching=0;
            for(int y=135;y<150;++y)for(int x=76;x<112;++x){auto p=pixels+(y*w+x)*4;if(std::abs(p[0]-c.r*255)<5&&std::abs(p[1]-c.g*255)<5&&std::abs(p[2]-c.b*255)<5)++matching;}
            stbi_image_free(pixels);check(matching>2,"screenshot finestra nascosta conserva etichetta CREA");
        }
        if(argc<=1)std::filesystem::remove(path);
    };
    shell.state().sketch_plane_picker=true;frame();
    auto plane=shell.state().camera.project({16,16,0},v);mouse(plane.x,plane.y,false);frame();snapshot("m3-plane-selection.png");
    mouse(plane.x,plane.y,true);mouse(plane.x,plane.y,false);
    check(!e.session().active_sketch.empty(),"clic sul piano crea schizzo");
    check(e.session().document.features.at(e.session().active_sketch).data["plane"]=="XY","clic sul piano XY seleziona XY");
    shell.state().view_transition.advance(shell.state().camera,1);frame();
    check(shell.state().sketch_mode,"modalità schizzo");snapshot("m3-sketch-mode.png");
    key(ImGuiKey_R);check(shell.state().sketch_tool.kind=="rect","scorciatoia R");
    auto origin=shell.state().camera.project({0,0,0},v);mouse(origin.x,origin.y,false);mouse(origin.x,origin.y,true);mouse(origin.x,origin.y,false);frame();
    check(shell.state().sketch_tool.started,"primo clic");
    ImGui::GetIO().AddInputCharactersUTF8("40");frame();key(ImGuiKey_Tab);frame();ImGui::GetIO().AddInputCharactersUTF8("20");frame();
    frame();snapshot("m3-live-dimensions.png");key(ImGuiKey_Enter);frame();
    auto result=e.execute("sketch.inspect");check(result.ok,"inspect");
    if(result.value["state"]!="constrained")std::cerr<<result.value.dump(2)<<'\n';
    check(result.value["state"]=="constrained","40 Tab20 Invio completamente vincolato");
    check(std::abs(result.value["points"][2]["x"].get<double>()-40)<1e-5&&std::abs(result.value["points"][2]["y"].get<double>()-20)<1e-5,"dimensioni effettive 40x20");
    check(result.value["profiles"].size()==1,"profilo chiuso");snapshot("m3-closed-profile.png");
    auto dimension_position=shell.state().camera.project({20,0,0},v);dimension_position.y-=ui::light.sketch.dimension_offset;
    mouse(dimension_position.x,dimension_position.y,false);mouse(dimension_position.x,dimension_position.y,true);mouse(dimension_position.x,dimension_position.y,false);mouse(dimension_position.x,dimension_position.y,true);mouse(dimension_position.x,dimension_position.y,false);
    check(shell.state().sketch_dimension_open&&!shell.state().sketch_dimension_id.empty(),"doppio clic modifica quota");
    shell.state().sketch_dimension_open=false;frame();
    auto id=e.session().active_sketch;check(e.execute("sketch.finish").ok,"fine");frame();check(!shell.state().sketch_mode,"chiude scheda contestuale");
    check(shell.open_feature_editor(id),"rientro dalla timeline");frame();check(shell.state().sketch_mode,"riapre schizzo");
    check(e.execute("sketch.create",{{"plane","XY"}}).ok,"schizzo libero");auto line=e.execute("sketch.line",{{"x1",2},{"y1",3},{"x2",12},{"y2",3}});check(line.ok,"linea libera");frame();shell.state().view_transition.advance(shell.state().camera,1);frame();
    auto previous_sketch_visible=[&](bool expected=true){
        frame();frame();std::string error;
        auto path=std::filesystem::temp_directory_path()/("sagomacad-sketch-visibility-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".png");
        check(shell.screenshot(path.string(),error),error.c_str());
        int w=0,h=0,channels=0;auto* pixels=stbi_load(path.string().c_str(),&w,&h,&channels,4);check(pixels,"screenshot visibilità leggibile");
        auto p=shell.state().camera.project({40,10,0},v);
        const auto c=light?ui::light.sketch.constrained:ui::dark.text;int matching=0;
        for(int y=int(p.y)-3;y<=int(p.y)+3;++y)for(int x=int(p.x)-3;x<=int(p.x)+3;++x)if(x>=0&&x<w&&y>=0&&y<h){
            auto pixel=pixels+(y*w+x)*4;if(std::abs(pixel[0]-c.r*255)<8&&std::abs(pixel[1]-c.g*255)<8&&std::abs(pixel[2]-c.b*255)<8)++matching;
        }
        stbi_image_free(pixels);std::filesystem::remove(path);
        check((matching>=3)==expected,"visibilità del primo schizzo coerente con modifica, soppressione e rollback");
    };
    shell.state().sketch_selection={"e1"};
    previous_sketch_visible();snapshot("sketch-second-visible.png");
    shell.state().sketch_selection.clear();
    auto start=shell.state().camera.project({2,3,0},v),end=shell.state().camera.project({5,7,0},v);
    const auto history=e.session().history.size();mouse(start.x,start.y,false);mouse(start.x,start.y,true);mouse(end.x,end.y,true);mouse(end.x+4,end.y-3,true);mouse(end.x+4,end.y-3,false);
    auto moved=e.execute("sketch.inspect");check(moved.ok,"inspect drag");
    check(std::abs(moved.value["points"][0]["y"].get<double>()-3)>1,"drag sposta punto libero");
    check(std::abs(moved.value["points"][0]["y"].get<double>()-moved.value["points"][1]["y"].get<double>())<1e-5,"drag conserva orizzontale");
    check(e.session().history.size()==history+1,"drag un solo undo");
    check(e.execute("sketch.finish").ok,"fine secondo schizzo");previous_sketch_visible();
    check(e.execute("feature.suppress",{{"feature_id",id},{"suppressed",true}}).ok,"sopprimi primo schizzo");previous_sketch_visible(false);
    check(e.execute("feature.suppress",{{"feature_id",id},{"suppressed",false}}).ok,"ripristina primo schizzo");previous_sketch_visible();
    check(e.execute("timeline.rollback",{{"index",0}}).ok,"rollback prima degli schizzi");previous_sketch_visible(false);
    check(e.execute("timeline.rollback",{{"index",e.session().document.timeline.size()}}).ok,"ripristina timeline");previous_sketch_visible();
    for(auto plane_name:{"XY","XZ","YZ"}){auto camera=ui::sketch_camera(ui::Camera{},plane_name);auto p=camera.project(ui::sketch_world(plane_name,{12,7}),v);engine::SketchXY xy;check(ui::sketch_cursor(camera,v,plane_name,p.x,p.y,xy),"unproject");check(std::abs(xy.x-12)<1e-8&&std::abs(xy.y-7)<1e-8,"projection roundtrip");}
    std::cout<<"UI: piano, R clic 40 Tab20 Invio, profilo, fine, riapertura OK\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
