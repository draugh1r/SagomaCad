#include "shell.hpp"
#include "ui/model_editor.hpp"
#include "ui/sketch_view.hpp"
#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
namespace sagomacad {
namespace {
ImU32 color(ui::Color c){return ImGui::ColorConvertFloat4ToU32({c.r,c.g,c.b,c.a});}
ImVec4 rgba(ui::Color c){return {c.r,c.g,c.b,c.a};}
double segment(ImVec2 p,ImVec2 a,ImVec2 b){double dx=b.x-a.x,dy=b.y-a.y,den=dx*dx+dy*dy,t=den>0?std::clamp(((p.x-a.x)*dx+(p.y-a.y)*dy)/den,0.,1.):0;return std::hypot(p.x-a.x-t*dx,p.y-a.y-t*dy);}
}
void Shell::draw_model_canvas(const ui::Theme& t,bool hovered){
    if(!engine_||state_.sketch_mode)return;auto& s=state_;auto& m=s.model;auto& io=ImGui::GetIO();auto* dl=ImGui::GetWindowDrawList();const float z=float(scale_);const auto& token=t.model;
    auto project=[&](kernel::Vec3 p){auto q=s.camera.project(p,current_viewport_);return ImVec2{float(q.x),float(q.y)};};
    auto inside=[&](float x,float y,float w,float h){return io.MousePos.x>=x&&io.MousePos.x<=x+w&&io.MousePos.y>=y&&io.MousePos.y<=y+h;};
    const auto panel=layout_.last().contextual;
    if(m.open&&(inside(panel.x,panel.y,panel.width,panel.height)||inside(m.value_x,m.value_y,token.field_width*z,token.field_height*z)))hovered=false;
    const auto context=engine::model_context(engine_->session().document,m.open?m.feature_id:"");const auto snapshot=regenerator_.evaluate(context);
    std::string hit_sketch,hit_region;double best=1e100;const auto preferred=m.open?m.sketch:s.selected_profile_sketch;
    struct Candidate {std::string sketch;const engine::ModelProfile* profile;};std::vector<Candidate> candidates;
    for(const auto& [id,views]:model_profiles_){auto solved=sketch_solutions_.find(id);if(solved==sketch_solutions_.end())continue;
        if(std::none_of(snapshot.features.begin(),snapshot.features.end(),[&](const auto& f){return f.id==id&&f.error.empty();}))continue;
        if(!regen::sketch_visible(context,snapshot,id,m.open?m.sketch:""))continue;
        engine::SketchXY cursor;const bool cursor_ok=hovered&&ui::sketch_cursor(s.camera,current_viewport_,solved->second.sketch.plane,io.MousePos.x,io.MousePos.y,cursor);
        for(const auto& view:views){candidates.push_back({id,&view});if(cursor_ok&&solver::contains(view.region,cursor.x,cursor.y)){auto p=s.camera.project(ui::sketch_world(solved->second.sketch.plane,cursor),current_viewport_);if(p.depth<best-1e-6||(std::abs(p.depth-best)<=1e-6&&id==preferred&&hit_sketch!=preferred)){best=p.depth;hit_sketch=id;hit_region=view.region.id;}}}
    }
    bool handle_hover=false;
    if(m.open&&!m.profiles.empty()&&model_preview_.surface&&(!m.revolve?m.extent==0:!model_preview_.angle_arc.empty())){
        auto start=model_preview_.handle,end=start,unit=start;
        if(!m.revolve){double factor=m.direction==1?.5:1;auto n=model_preview_.normal;end={start.x+n.x*model_preview_.amount*factor,start.y+n.y*model_preview_.amount*factor,start.z+n.z*model_preview_.amount*factor};unit={end.x+n.x*factor,end.y+n.y*factor,end.z+n.z*factor};}
        else {end=model_preview_.angle_arc.back();auto n=model_preview_.normal;unit={end.x+n.x,end.y+n.y,end.z+n.z};for(size_t i=1;i<model_preview_.angle_arc.size();++i)dl->AddLine(project(model_preview_.angle_arc[i-1]),project(model_preview_.angle_arc[i]),color(t.accent),t.sketch.line_width*z);start=model_preview_.angle_arc.front();}
        auto a=project(start),b=project(end),u=project(unit);m.handle_x=b.x;m.handle_y=b.y;
        const double dx=b.x-a.x,dy=b.y-a.y,length=std::hypot(dx,dy);const float nx=length>1?float(dx/length):0,ny=length>1?float(dy/length):-1;
        if(!m.revolve){dl->AddLine(a,b,color(t.panel),token.arrow_width*z*.7f);dl->AddLine(a,b,color(t.accent),token.arrow_width*z*.4f);
            ImVec2 left{b.x-nx*token.arrow_length*z-ny*token.arrow_width*z,b.y-ny*token.arrow_length*z+nx*token.arrow_width*z},right{b.x-nx*token.arrow_length*z+ny*token.arrow_width*z,b.y-ny*token.arrow_length*z-nx*token.arrow_width*z};dl->AddTriangleFilled(b,left,right,color(t.accent));}
        else {dl->AddCircleFilled(b,token.arrow_width*z,color(t.panel));dl->AddCircleFilled(b,token.arrow_width*z*.7f,color(t.accent));}
        handle_hover=hovered&&(std::hypot(io.MousePos.x-b.x,io.MousePos.y-b.y)<token.handle_hit*z||(!m.revolve&&segment(io.MousePos,a,b)<token.handle_hit*z*.5));
        if(handle_hover&&ImGui::IsMouseClicked(0)){m.dragging=true;m.drag_x=io.MousePos.x;m.drag_y=io.MousePos.y;m.drag_amount=model_preview_.amount;m.drag_px=u.x-b.x;m.drag_py=u.y-b.y;}
        if(m.dragging){if(ImGui::IsMouseDown(0)){auto value=engine::model_drag_value(m.drag_amount,io.MousePos.x-m.drag_x,io.MousePos.y-m.drag_y,m.drag_px,m.drag_py,m.revolve);auto& field=m.revolve?m.angle:m.distance;std::snprintf(field.data(),field.size(),m.revolve?"%.3f deg":"%.3f mm",value);if(m.revolve)m.type=0;m.error.clear();}else m.dragging=false;}
        const auto v=current_viewport_;
        m.value_x=std::clamp(b.x+token.value_gap*z,float(v.x)+t.padding*z,float(v.x+v.width)-token.field_width*z-t.padding*z);
        m.value_y=std::clamp(b.y+token.value_gap*z,float(v.y)+t.padding*z,float(v.y+v.height)-token.field_height*z-t.padding*z);
        if(m.value_x+token.field_width*z>panel.x&&m.value_y+token.field_height*z>panel.y&&m.value_y<panel.y+panel.height)m.value_x=panel.x-token.field_width*z-token.value_gap*z;
    }
    if(m.open&&m.pick_axis){
        std::string axis_hit;double nearest=token.handle_hit*z;
        auto axis_line=[&](const std::string& id,kernel::Vec3 a,kernel::Vec3 b,ui::Color tint){auto p=project(a),q=project(b);double d=segment(io.MousePos,p,q);if(hovered&&d<nearest){nearest=d;axis_hit=id;}dl->AddLine(p,q,color(m.axis==id?token.axis:tint),m.axis==id?3*z:t.sketch.line_width*z);dl->AddText(q,color(tint),id.c_str());};
        const double e=token.axis_extent;axis_line("X",{-e,0,0},{e,0,0},t.axis_x);axis_line("Y",{0,-e,0},{0,e,0},t.axis_y);axis_line("Z",{0,0,-e},{0,0,e},t.cube.axis_z);
        auto solved=sketch_solutions_.find(m.sketch);if(solved!=sketch_solutions_.end())for(const auto& line:solved->second.sketch.entities)if(line.kind=="line"){
            auto point=[&](const std::string& id){for(const auto& p:solved->second.sketch.points)if(p.id==id)return ui::sketch_world(solved->second.sketch.plane,{p.x,p.y});return kernel::Vec3{};};axis_line(line.id,point(line.points[0]),point(line.points[1]),t.muted);
        }
        if(hovered&&!axis_hit.empty()){ImGui::SetTooltip("Asse %s",axis_hit.c_str());if(ImGui::IsMouseClicked(0)){m.axis=axis_hit;m.pick_axis=false;m.error.clear();}}
    }else if(hovered&&!handle_hover&&!m.dragging&&ImGui::IsMouseClicked(0)&&!hit_region.empty()){
        auto& sketch=m.open?m.sketch:s.selected_profile_sketch;auto& selected=m.open?m.profiles:s.selected_profiles;
        if(sketch!=hit_sketch){sketch=hit_sketch;selected.clear();}if(!m.open&&!io.KeyCtrl)selected.clear();
        auto it=std::find(selected.begin(),selected.end(),hit_region);if(it==selected.end())selected.push_back(hit_region);else selected.erase(it);m.error.clear();
    }
    for(const auto& candidate:candidates){const auto& view=*candidate.profile;const auto& ids=m.open?m.profiles:s.selected_profiles;const bool selected=(m.open?m.sketch:s.selected_profile_sketch)==candidate.sketch&&std::find(ids.begin(),ids.end(),view.region.id)!=ids.end();
        auto tint=selected?token.selected:t.sketch.profile;if(!m.pick_axis&&candidate.sketch==hit_sketch&&view.region.id==hit_region&&!selected)tint.a*=1.7f;
        const auto flags=dl->Flags;dl->Flags&=~ImDrawListFlags_AntiAliasedFill;
        for(const auto& triangle:view.fill.triangles)dl->AddTriangleFilled(project(triangle.vertices[0]),project(triangle.vertices[1]),project(triangle.vertices[2]),color(tint));
        dl->Flags=flags;
        if(selected)for(const auto& edge:view.fill.edges)for(size_t i=1;i<edge.points.size();++i)dl->AddLine(project(edge.points[i-1]),project(edge.points[i]),color(t.accent),t.sketch.line_width*z);
    }
    if(m.open&&!m.axis.empty()&&!m.pick_axis&&m.revolve&&model_preview_.surface){auto o=model_preview_.axis_origin,n=model_preview_.axis_direction;double e=token.axis_extent;dl->AddLine(project({o.x-n.x*e,o.y-n.y*e,o.z-n.z*e}),project({o.x+n.x*e,o.y+n.y*e,o.z+n.z*e}),color(token.axis),t.sketch.line_width*z*1.5f);}
    if(m.open&&!m.profiles.empty()&&model_preview_.surface&&(!m.revolve?m.extent==0:!model_preview_.angle_arc.empty())){
        ImGui::SetCursorScreenPos({m.value_x,m.value_y});ImGui::SetNextItemWidth(token.field_width*z);auto& field=m.revolve?m.angle:m.distance;
        if(ImGui::InputText("##model_value",field.data(),field.size(),ImGuiInputTextFlags_AutoSelectAll))m.error.clear();
        if(ImGui::IsItemActivated()&&m.revolve)m.type=0;
    }
    if(m.open&&ImGui::IsKeyPressed(ImGuiKey_Escape)){if(m.dragging)m.dragging=false;else if(m.pick_axis)m.pick_axis=false;else ui::cancel_model(s);}
}
void Shell::draw_model_panel(const ui::Theme& t){
    auto& m=state_.model;if(!m.open||!engine_)return;const float z=float(scale_);const auto r=layout_.last().contextual;if(!r.visible())return;
    ImGui::SetNextWindowPos({r.x,r.y});ImGui::SetNextWindowSize({r.width,r.height});
    constexpr auto flags=ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoDocking;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,t.rounding*z);
    if(ImGui::Begin("##model_panel",nullptr,flags)){
        m.fields.clear();ImGui::TextUnformatted(m.revolve?"RIVOLUZIONE":"ESTRUDI");ImGui::Separator();
        const float content=std::max(t.model.row*z,ImGui::GetContentRegionAvail().y-t.model.footer*z);
        ImGui::BeginChild("model_fields",{0,content},ImGuiChildFlags_None,ImGuiWindowFlags_NoBackground);
        auto record=[&](const char* key){auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();m.fields[key]={a.x,a.y,b.x-a.x,b.y-a.y};};
        if(ImGui::BeginTable("model_parameters",2,ImGuiTableFlags_SizingStretchProp)){
            ImGui::TableSetupColumn("label",ImGuiTableColumnFlags_WidthFixed,t.model.label_width*z);ImGui::TableSetupColumn("value",ImGuiTableColumnFlags_WidthStretch);
            auto row=[&](const char* label){ImGui::TableNextRow(ImGuiTableRowFlags_None,t.model.row*z);ImGui::TableSetColumnIndex(0);ImGui::AlignTextToFramePadding();ImGui::TextUnformatted(label);ImGui::TableSetColumnIndex(1);ImGui::SetNextItemWidth(-1);};
            if(!m.revolve){row("Tipo");ImGui::TextUnformatted("Solido");}
            row("Profilo");auto profile_label=m.profiles.empty()?std::string("Seleziona"):std::to_string(m.profiles.size())+(m.profiles.size()==1?" selezionato":" selezionati");
            if(ImGui::Button((profile_label+"##profile").c_str(),{-t.model.row*z,0}))m.pick_axis=false;record("profile");ImGui::SameLine();if(ImGui::SmallButton("x##clear_profile"))m.profiles.clear();
            if(m.revolve){
                row("Asse");const bool picking=m.pick_axis;if(picking)ImGui::PushStyleColor(ImGuiCol_Button,rgba(t.model.selected));
                if(ImGui::Button(((m.axis.empty()?std::string("Seleziona"):m.axis)+"##axis").c_str(),{-t.model.row*z,0}))m.pick_axis=true;record("axis");if(picking)ImGui::PopStyleColor();ImGui::SameLine();if(ImGui::SmallButton("x##clear_axis")){m.axis.clear();m.pick_axis=true;}
                row("Tipo");if(ImGui::Combo("##revolve_type",&m.type,"Angolo\0Completo\0")&&m.type==1)std::snprintf(m.angle.data(),m.angle.size(),"360 deg");record("type");
                row("Angolo");ImGui::BeginDisabled(m.type==1);ImGui::InputText("##angle",m.angle.data(),m.angle.size());record("angle");ImGui::EndDisabled();
                row("Direzione");ImGui::TextUnformatted("Un lato");
            }else {
                row("Inizio");ImGui::TextUnformatted("Piano profilo");
                row("Direzione");ImGui::Combo("##direction",&m.direction,"Un lato\0Simmetrica\0Due lati\0");record("direction");
                row("Estensione");ImGui::Combo("##extent",&m.extent,"Distanza\0Attraverso tutto\0");record("extent");
                row("Distanza");ImGui::BeginDisabled(m.extent==1);ImGui::InputText("##distance",m.distance.data(),m.distance.size());record("distance");ImGui::EndDisabled();
                if(m.direction==2){row("Distanza 2");ImGui::BeginDisabled(m.extent==1);ImGui::InputText("##second_distance",m.second_distance.data(),m.second_distance.size());record("second_distance");ImGui::EndDisabled();}
                row("Angolo sformo");ImGui::TextDisabled("0 deg");
            }
            ImGui::TableNextRow(ImGuiTableRowFlags_None,t.spacing*z);ImGui::TableSetColumnIndex(1);ImGui::Separator();
            row("Operazione");const char* operations[]={"Unisci","Taglia","Interseca","Nuovo corpo"};const char* icons[]={"blend","package-open","scan-line","box"};
            auto position=ImGui::GetCursorScreenPos();m.fields["operation"]={position.x,position.y,ImGui::GetContentRegionAvail().x,ImGui::GetFrameHeight()};
            m.operation_menu=ImGui::BeginCombo("##operation",operations[std::clamp(m.operation,0,3)]);
            if(m.operation_menu){for(int i=0;i<4;++i){if(ImGui::Selectable(("##op"+std::to_string(i)).c_str(),m.operation==i,ImGuiSelectableFlags_None,{0,t.model.row*z})){
                        m.operation=i;m.error.clear();if(i!=3&&m.targets.empty()){auto context=engine::model_context(engine_->session().document,m.feature_id);auto snapshot=regen::Regenerator{}.evaluate(context);for(const auto& body:snapshot.bodies)if(body.exact)m.targets.push_back(body.id);}}
                    record(("op"+std::to_string(i)).c_str());auto a=ImGui::GetItemRectMin();auto texture=icon_texture(icons[i]);if(texture)ImGui::GetWindowDrawList()->AddImage((ImTextureID)(intptr_t)texture,{a.x+t.spacing*z,a.y+t.spacing*z*.5f},{a.x+(t.spacing+t.timeline.icon)*z,a.y+(t.spacing*.5f+t.timeline.icon)*z},{0,0},{1,1},color(t.text));
                    ImGui::GetWindowDrawList()->AddText({a.x+(t.timeline.icon+t.spacing*2)*z,a.y+t.spacing*z*.5f},color(t.text),operations[i]);}
                ImGui::EndCombo();}
            if(m.operation!=3){
                row("Corpi");auto label=m.targets.empty()?std::string("Seleziona"):std::to_string(m.targets.size())+(m.targets.size()==1?" selezionato":" selezionati");
                if(ImGui::BeginCombo("##targets",label.c_str())){auto context=engine::model_context(engine_->session().document,m.feature_id);const auto snapshot=regen::Regenerator{}.evaluate(context);
                    for(const auto& body:context.bodies){auto shape=std::find_if(snapshot.bodies.begin(),snapshot.bodies.end(),[&](const auto& b){return b.id==body.id;});if(shape==snapshot.bodies.end()||!shape->exact)continue;auto found=std::find(m.targets.begin(),m.targets.end(),body.id);bool checked=found!=m.targets.end();if(ImGui::Checkbox(body.name.c_str(),&checked)){if(checked)m.targets.push_back(body.id);else m.targets.erase(found);}}
                    ImGui::EndCombo();}record("targets");
            }
            ImGui::EndTable();
        }
        const auto error=m.error.empty()?m.preview_error:m.error;if(!error.empty()){ImGui::PushStyleColor(ImGuiCol_Text,rgba(t.error));ImGui::TextWrapped("%s",error.c_str());ImGui::PopStyleColor();}
        ImGui::EndChild();ImGui::Separator();
        const bool disabled=!m.preview_error.empty()||m.profiles.empty()||(m.revolve&&m.axis.empty());
        ImGui::SetCursorPosX(std::max(t.padding*z,ImGui::GetWindowWidth()-ImGui::GetStyle().WindowPadding.x-t.model.row*z*4.5f-ImGui::GetStyle().ItemSpacing.x));
        ImGui::BeginDisabled(disabled);if(ImGui::Button("OK",{t.model.row*z*2,0}))ui::submit_model(state_,*engine_);record("ok");ImGui::EndDisabled();ImGui::SameLine();
        if(ImGui::Button("Annulla",{t.model.row*z*2.5f,0}))ui::cancel_model(state_);record("cancel");
    }
    ImGui::End();ImGui::PopStyleVar();
}
}
