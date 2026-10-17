#include "ui/surface_editor.hpp"
#include <cstdio>
namespace sagomacad::ui {
namespace {void copy(auto& b,const std::string& s){std::snprintf(b.data(),b.size(),"%s",s.c_str());}}
void begin_surface(State& s,engine::Engine& e,const std::string& command,const std::string& editing){
    s.surface={};auto& m=s.surface;m.open=true;m.command=command;m.feature_id=editing;copy(m.value,"3 mm");copy(m.diameter,"8 mm");copy(m.depth,"10 mm");
    if(!e.session().active_sketch.empty())e.execute("sketch.finish");s.sketch_mode=false;s.model.open=false;s.sketch_plane_picker=false;s.primitive_create_open=false;s.primitive_editor_open=false;s.navigation_mode=-1;s.selected_profiles.clear();s.selected_profile_sketch.clear();
    auto selection=e.execute("selection.get");if(selection.ok)for(const auto& ref:selection.value["refs"]){auto kind=ref.value("kind",std::string());if(command=="feature.hole"&&kind=="face"&&m.face.is_null())m.face=ref;if(command!="feature.hole"&&kind=="edge")m.edges.push_back(ref);}
    if(!editing.empty()){
        auto f=e.session().document.features.find(editing);if(f==e.session().document.features.end()){m.error="Feature mancante";return;}const auto& p=f->second.data;
        if(command=="feature.hole"){m.face=p.at("face");m.position=p.at("position");m.references=p.at("references");m.through=p.value("through_all",false);copy(m.diameter,p.at("diameter"));copy(m.depth,p.at("depth"));for(int i=0;i<2&&i<int(m.references.size());++i)copy(m.offsets[i],m.references[i].at("distance"));}
        else{m.edges=p.at("edges");copy(m.value,p.at(command=="feature.fillet"?"radius":"distance"));}
    }
}
nlohmann::json surface_params(const State& s){const auto& m=s.surface;if(m.command!="feature.hole")return {{"edges",m.edges},{m.command=="feature.fillet"?"radius":"distance",m.value.data()}};
    nlohmann::json p={{"face",m.face},{"diameter",m.diameter.data()},{"depth",m.depth.data()},{"through_all",m.through}};if(!m.position.is_null())p["position"]=m.position;
    if(m.references.size()==2){p["references"]=m.references;for(int i=0;i<2;++i)p["references"][i]["distance"]=m.offsets[i].data();}return p;
}
bool submit_surface(State& s,engine::Engine& e){auto& m=s.surface;auto p=surface_params(s);auto r=m.feature_id.empty()?e.execute(m.command,p):e.execute("feature.edit",{{"feature_id",m.feature_id},{"params",p}});m.error=r.error;if(r.ok){m.open=false;e.execute("selection.set",{{"refs",nlohmann::json::array()}});}return r.ok;}
}
