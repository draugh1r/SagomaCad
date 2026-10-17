#include "ui/model_editor.hpp"
#include <algorithm>
#include <cstdio>
#include <cmath>
namespace sagomacad::ui {
namespace {void copy(auto& buffer,const std::string& value){std::snprintf(buffer.data(),buffer.size(),"%s",value.c_str());}}
void begin_model(State& s,engine::Engine& e,bool revolve,const std::string& editing){
    s.surface.open=false;s.model={};auto& m=s.model;m.open=true;m.revolve=revolve;m.feature_id=editing;
    copy(m.distance,"10 mm");copy(m.second_distance,"10 mm");copy(m.angle,"360 deg");
    m.sketch=e.session().active_sketch.empty()?s.selected_profile_sketch:e.session().active_sketch;
    if(m.sketch==s.selected_profile_sketch)m.profiles=s.selected_profiles;
    const auto source=e.session().document.features.find(m.sketch);
    if(source==e.session().document.features.end()||source->second.type!="sketch"||source->second.suppressed){m.sketch.clear();m.profiles.clear();}
    else {
        const auto inspected=e.execute("sketch.inspect",{{"sketch",m.sketch}});
        if(inspected.ok){const auto& regions=inspected.value["regions"];
            std::erase_if(m.profiles,[&](const auto& id){return std::none_of(regions.begin(),regions.end(),[&](const auto& r){return r["id"]==id;});});
            if(m.profiles.empty()&&regions.size()==1)m.profiles.push_back(regions[0]["id"].get<std::string>());
        }else m.profiles.clear();
    }
    if(!revolve&&m.profiles.empty()&&e.session().active_sketch.empty()){
        auto selection=e.execute("selection.get");if(selection.ok)for(const auto& ref:selection.value["refs"])if(ref.value("kind",std::string())=="face"){m.face=ref;break;}
    }
    if(!e.session().active_sketch.empty())e.execute("sketch.finish");
    s.sketch_mode=false;s.sketch_plane_picker=false;s.primitive_editor_open=false;s.primitive_create_open=false;s.navigation_mode=-1;
    const auto forward=s.camera.forward();
    if(editing.empty()&&std::max({std::abs(forward.x),std::abs(forward.y),std::abs(forward.z)})>.98){auto destination=s.camera;destination.home();destination.target=s.camera.target;destination.distance=s.camera.distance;s.view_transition.begin(s.camera,destination,theme(s.theme=="light").cube.transition_duration);}
    if(!editing.empty()){
        auto f=e.session().document.features.find(editing);if(f==e.session().document.features.end()){m.error="Feature mancante";return;}
        try {
        const auto& p=f->second.data;m.face=p.value("face",nlohmann::json());m.sketch=p.value("sketch",std::string());m.profiles.clear();
        for(const auto& ref:p.value("profiles",nlohmann::json::array())){if(ref.is_string())m.profiles.push_back(ref.get<std::string>());else {auto ids=ref.get<std::vector<std::string>>();std::sort(ids.begin(),ids.end());m.profiles.push_back("region:"+nlohmann::json(ids).dump());}}
        m.targets=p.value("target_bodies",std::vector<std::string>{});
        auto index=[](std::string v,std::initializer_list<const char*> values){int i=0;for(auto x:values){if(v==x)return i;++i;}return 0;};
        m.operation=index(p.value("operation",std::string("new")),{"join","cut","intersect","new"});
        if(revolve){m.axis=p.value("axis",std::string());m.type=p.value("type",std::string("angle"))=="full"?1:0;copy(m.angle,m.type==1?std::string("360 deg"):p.value("angle",std::string("360 deg")));}
        else {m.direction=index(p.value("direction",std::string("one_side")),{"one_side","symmetric","two_sides"});m.extent=p.value("extent",std::string("distance"))=="through_all"?1:0;copy(m.distance,p.value("distance",std::string("10 mm")));copy(m.second_distance,p.value("second_distance",std::string("10 mm")));}
        }catch(const std::exception& error){m.error=error.what();}
    }
}
nlohmann::json model_params(const State& s){
    const auto& m=s.model;const char* ops[]={"join","cut","intersect","new"};
    nlohmann::json p={{"sketch",m.sketch},{"profiles",m.profiles},{"operation",ops[std::clamp(m.operation,0,3)]},{"target_bodies",m.operation==3?std::vector<std::string>{}:m.targets}};
    if(m.face.is_object()&&!m.revolve){p.erase("sketch");p.erase("profiles");p["face"]=m.face;}
    if(m.revolve){p["axis"]=m.axis;p["type"]=m.type==1?"full":"angle";p["angle"]=m.angle.data();}
    else {const char* dirs[]={"one_side","symmetric","two_sides"};p["distance"]=m.distance.data();p["second_distance"]=m.second_distance.data();p["direction"]=dirs[std::clamp(m.direction,0,2)];p["extent"]=m.extent==1?"through_all":"distance";}
    return p;
}
bool submit_model(State& s,engine::Engine& e){auto& m=s.model;auto params=model_params(s);auto r=m.feature_id.empty()?e.execute(m.revolve?"feature.revolve":"feature.extrude",params):e.execute("feature.edit",{{"feature_id",m.feature_id},{"params",params}});m.error=r.error;if(r.ok){m.open=false;s.selected_profiles.clear();s.selected_profile_sketch.clear();}return r.ok;}
void cancel_model(State& s){s.model.open=false;s.model.dragging=false;s.model.pick_axis=false;s.selected_profile_sketch.clear();s.selected_profiles.clear();}
nlohmann::json model_state_json(const State& s){const auto& m=s.model;return {{"open",m.open},{"revolve",m.revolve},{"feature_id",m.feature_id},{"params",model_params(s)},{"pick_axis",m.pick_axis},{"fields",m.fields},{"preview_error",m.preview_error},{"handle",{{"x",m.handle_x},{"y",m.handle_y}}},{"value",{{"x",m.value_x},{"y",m.value_y}}}};}
}
