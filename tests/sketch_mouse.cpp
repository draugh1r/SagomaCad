#include "shell.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
using namespace sagomacad;
void require(bool b,const char* message){if(!b)throw std::runtime_error(message);}
int main(int argc,char** argv){try{
    int width=1440,height=900;bool light=true;
    if(argc>2){std::string size=argv[2];auto x=size.find('x');width=std::stoi(size.substr(0,x));height=std::stoi(size.substr(x+1));}
    if(argc>3)light=std::string(argv[3])!="dark";
    engine::Engine engine;Shell shell(width,height,1,true,light);require(shell.valid(),"shell");shell.bind_engine(&engine);
    auto frame=[&](){shell.render();};frame();frame();frame();
    auto* window=ImGui::FindWindowByName("Viewport");ui::Viewport v{window->WorkRect.Min.x,window->WorkRect.Min.y,window->WorkRect.GetWidth(),window->WorkRect.GetHeight()};
    auto mouse=[&](double x,double y,bool down){auto p=shell.state().camera.project({x,y,0},v);ImGui::GetIO().AddMousePosEvent(float(p.x),float(p.y));ImGui::GetIO().AddMouseButtonEvent(0,down);frame();};
    auto click=[&](double x,double y){mouse(x,y,false);mouse(x,y,true);mouse(x,y,false);};
    auto key=[&](ImGuiKey k){ImGui::GetIO().AddKeyEvent(k,true);frame();ImGui::GetIO().AddKeyEvent(k,false);frame();};
    auto start=[&](const char* kind,ImGuiKey shortcut){
        require(engine.execute("sketch.create",{{"plane","XY"}}).ok,"create");frame();shell.state().view_transition.advance(shell.state().camera,1);frame();
        if(shortcut!=ImGuiKey_None)key(shortcut);else shell.state().sketch_tool.begin(kind);
        click(0,0);frame();require(shell.state().sketch_tool.started,"primo clic inizia il disegno");
    };
    auto inspect=[&](){auto r=engine.execute("sketch.inspect");require(r.ok,"inspect");return r.value;};
    auto snapshot=[&](const char* name){if(argc>1){frame();frame();std::string error;require(shell.screenshot(std::string(argv[1])+"/"+name,error),error.c_str());}};
    // SDL/ImGui quantize pointer positions to framebuffer pixels.
    auto close=[&](double a,double b){return std::abs(a-b)<2*shell.state().camera.distance/(v.height*1.2071067811865475);};
    for(const auto& kind:{std::string("line"),std::string("rect"),std::string("circle")}){
        start(kind.c_str(),kind=="line"?ImGuiKey_L:kind=="rect"?ImGuiKey_R:ImGuiKey_C);
        require(ImGui::GetIO().WantTextInput,"focus quota presente durante il disegno");
        mouse(10,-10,false);frame();
        require(close(shell.state().sketch_tool.cursor_x,10)&&close(shell.state().sketch_tool.cursor_y,-10),"caselle non intercettano il puntatore nel viewport");
        mouse(12,9,false);frame();auto& state=shell.state();
        if(!close(state.sketch_tool.cursor_x,12)||!close(state.sketch_tool.cursor_y,9)){
            const auto* g=ImGui::GetCurrentContext();std::cerr<<kind<<" cursor="<<state.sketch_tool.cursor_x<<","<<state.sketch_tool.cursor_y<<" mouse="<<g->IO.MousePos.x<<","<<g->IO.MousePos.y<<" hovered="<<(g->HoveredWindow?g->HoveredWindow->Name:"none")<<" active="<<(g->ActiveIdWindow?g->ActiveIdWindow->Name:"none")<<'\n';
        }
        require(close(state.sketch_tool.cursor_x,12)&&close(state.sketch_tool.cursor_y,9),"cursore segue mouse anche con input attivo");
        double first=std::strtod(state.sketch_fields[0].data(),nullptr);
        require(close(first,kind=="line"?15:kind=="rect"?12:30),"input mostra misura corrente");
        require(!state.sketch_tool.locked[0]&&!state.sketch_tool.locked[1],"visualizzare una misura non la blocca");
        mouse(20,15,false);frame();require(!close(first,std::strtod(state.sketch_fields[0].data(),nullptr)),"misura cambia con il mouse");
        require(inspect()["entities"].empty(),"anteprima non modifica il documento");snapshot(("mouse-"+kind+"-preview.png").c_str());
        click(20,15);frame();auto model=inspect();require(!state.sketch_tool.started,"secondo clic conferma senza dialog successivo");
        require(model["dimensions"].empty(),"disegno mouse non crea quote bloccate");
        if(kind=="circle")require(close(model["entities"][0]["radius"].get<double>(),25),"raggio da mouse");
        else require(close(model["points"][kind=="line"?1:2]["x"].get<double>(),20)&&close(model["points"][kind=="line"?1:2]["y"].get<double>(),15),"geometria confermata sul secondo clic");
        require(model["state"]=="free","geometria mouse rimane modificabile");
    }
    start("rect",ImGuiKey_R);mouse(12,9,false);frame();
    ImGui::GetIO().AddInputCharactersUTF8("40");frame();frame();
    require(shell.state().sketch_tool.locked[0]&&!shell.state().sketch_tool.locked[1],"solo larghezza digitata bloccata");
    mouse(20,12,false);frame();
    require(close(std::strtod(shell.state().sketch_fields[0].data(),nullptr),40)&&close(std::strtod(shell.state().sketch_fields[1].data(),nullptr),12),"larghezza ferma, altezza segue mouse");
    snapshot("mouse-rectangle-width-locked.png");click(20,12);frame();auto rect=inspect();
    require(rect["dimensions"].size()==1&&rect["degrees_of_freedom"]==1,"una sola quota reale");
    require(close(rect["points"][2]["x"].get<double>(),40)&&close(rect["points"][2]["y"].get<double>(),12),"clic conferma quota e misura libera");
    start("arc",ImGuiKey_None);mouse(3,4,false);frame();require(inspect()["entities"].empty(),"arco anteprima centro-punto");click(3,4);
    mouse(0,8,false);frame();require(close(std::strtod(shell.state().sketch_fields[0].data(),nullptr),5),"arco conserva raggio scelto con mouse");click(0,8);frame();
    auto arc=inspect();require(arc["entities"].size()==1&&arc["entities"][0]["kind"]=="arc"&&arc["dimensions"].empty(),"arco confermato con mouse senza quote imposte");
    std::cout<<"Mouse: anteprima e misure live, clic-clic, quote facoltative e blocco parziale OK\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
