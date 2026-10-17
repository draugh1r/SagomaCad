#include "shell.hpp"
#include "ui/frame.hpp"
#include <iostream>
#include <stdexcept>
using namespace sagomacad;
void check(bool b,const std::string& m){if(!b)throw std::runtime_error(m);}
void limits(const Shell& shell) {
    const auto display=ImGui::GetIO().DisplaySize;
    size_t checked=0;
    for(const auto& r:ui::frame::records()) {
        if(r.maximum==0)continue;
        ++checked;
        check(r.rect.width<=r.maximum+.1f&&r.rect.width>=r.minimum-.1f,"width limits: "+r.name);
        check(r.rect.x>=-.1f&&r.rect.y>=-.1f&&r.rect.x+r.rect.width<=display.x+.1f&&r.rect.y+r.rect.height<=display.y+.1f,"on screen: "+r.name);
    }
    check(checked>0,"expected dialog/window records");
}
int main()try {
    for(bool light:{true,false})for(auto size:{std::pair{1440,900},std::pair{1280,720}})for(int scale:{1,2}) {
        engine::Engine engine;
        Shell shell(size.first,size.second,scale,true,light);check(shell.valid(),shell.error());shell.bind_engine(&engine);
        auto& state=shell.state();state.primitive_create_open=true;
        for(int i=0;i<3;++i)shell.render();limits(shell);
        state.primitive_create_open=false;state.palette_open=true;state.parameters_open=true;state.file_dialog_open=true;
        for(int i=0;i<3;++i)shell.render();limits(shell);
        state.palette_open=state.parameters_open=state.file_dialog_open=false;
        auto sketch=engine.execute("sketch.create",{{"plane","XY"}});check(sketch.ok,sketch.error);
        for(int i=0;i<3;++i)shell.render();limits(shell);
        state.sketch_dimension_open=state.sketch_constraint_open=true;
        for(int i=0;i<3;++i)shell.render();limits(shell);
    }
    std::cout<<"Dialog and window bounds: both themes, two sizes, two DPI scales\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
