#include "shell.hpp"
#include "ui/surface_editor.hpp"
#include "ui/sketch_view.hpp"
#include "engine/tools/model_preview.hpp"
#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <set>
namespace sagomacad {
namespace {
using J=nlohmann::json;
ImU32 color(ui::Color c){return ImGui::ColorConvertFloat4ToU32({c.r,c.g,c.b,c.a});}
void copy(auto& b,const std::string& s){std::snprintf(b.data(),b.size(),"%s",s.c_str());}
const char* kind(engine::PickKind k){return k==engine::PickKind::Face?"face":k==engine::PickKind::Edge?"edge":k==engine::PickKind::Body?"body":"sketch";}
}
void Shell::draw_geometry_canvas(const ui::Theme& t,bool hovered){
    if(!engine_||state_.primitive_create_open||state_.primitive_editor_open)return;auto& s=state_;auto& m=s.surface;auto& io=ImGui::GetIO();auto* dl=ImGui::GetWindowDrawList();const float z=float(scale_);auto v=current_viewport_;const auto panel=layout_.last().contextual;
    auto inside=[&](float x,float y,float w,float h){return io.MousePos.x>=x&&io.MousePos.x<=x+w&&io.MousePos.y>=y&&io.MousePos.y<=y+h;};
    if(s.model.open&&!s.model.face.is_object()&&!s.model.profiles.empty())hovered=false;
    if((m.open||s.model.open)&&inside(panel.x,panel.y,panel.width,panel.height))hovered=false;
    if(m.open&&inside(m.value_x,m.value_y,t.model.field_width*z,t.model.field_height*z))hovered=false;
    auto project=[&](kernel::Vec3 p){auto q=s.camera.project(p,v);return ImVec2{float(q.x),float(q.y)};};
    const auto context=engine::model_context(engine_->session().document,m.open?m.feature_id:s.model.open?s.model.feature_id:"");
    const auto document_revision=doc::to_json(context).dump();
    if(document_revision!=topology_revision_){topology_revision_=document_revision;engine::Engine query;query.session().document=context;auto r=query.execute("geometry.query",{{"limit",16384}});topology_entities_=r.ok?r.value.at("entities"):J::array();}
    auto filter=s.pick_filter;if(m.open){filter.faces=m.command=="feature.hole";filter.edges=!filter.faces;filter.bodies=false;filter.sketches=false;}
    auto camera=J::array({s.camera.target.x,s.camera.target.y,s.camera.target.z,s.camera.yaw,s.camera.pitch,s.camera.roll,s.camera.distance,v.width,v.height,filter.faces,filter.edges,filter.bodies,filter.sketches});
    auto revision=surface_revision_+camera.dump();
    if(revision!=pick_revision_&&v.width>0&&v.height>0){pick_revision_=revision;std::vector<engine::PickTriangle> triangles;std::vector<engine::PickSegment> segments;
        auto point=[&](kernel::Vec3 p){auto q=s.camera.project(p,v);return engine::PickPoint{q.x-v.x,q.y-v.y,q.depth};};
        for(size_t index=0;index<surfaces_.size()&&index<surface_bodies_.size();++index){const auto& surface=surfaces_[index];auto owner=surface_bodies_[index];
            for(const auto& tri:surface.geometry.triangles){engine::PickTriangle p{{point(tri.vertices[0]),point(tri.vertices[1]),point(tri.vertices[2])},{engine::PickKind::Face,owner,tri.face_id}};if(std::all_of(p.points.begin(),p.points.end(),[](auto q){return q.depth>.01;}))triangles.push_back(p);}
            for(const auto& edge:surface.geometry.edges)for(size_t i=1;i<edge.points.size();++i){engine::PickSegment p{{point(edge.points[i-1]),point(edge.points[i])},{engine::PickKind::Edge,owner,edge.edge_id}};if(p.points[0].depth>.01&&p.points[1].depth>.01)segments.push_back(p);}
        }
        if(filter.sketches&&!s.sketch_mode){auto snap=regen::Regenerator{}.evaluate(context);for(const auto& [id,solved]:sketch_solutions_)if(regen::sketch_visible(context,snap,id))for(const auto& stroke:engine::sketch_overlay(solved).strokes)for(size_t i=1;i<stroke.points.size();++i){engine::PickSegment segment{{point(ui::sketch_world(solved.sketch,stroke.points[i-1])),point(ui::sketch_world(solved.sketch,stroke.points[i]))},{engine::PickKind::Sketch,id,0}};if(segment.points[0].depth>.01&&segment.points[1].depth>.01)segments.push_back(segment);}}
        try{pick_buffer_.build(int(v.width),int(v.height),std::move(triangles),std::move(segments),filter,t.selection.edge_hit*z);}catch(const std::exception& e){s.status_message=e.what();}
    }
    auto reference=[&](const engine::PickRef& p)->J{
        if(p.kind==engine::PickKind::Sketch)return {{"kind","sketch"},{"body",""},{"name",p.owner}};
        if(p.kind==engine::PickKind::Body)return {{"kind","body"},{"body",p.owner},{"name",p.owner}};
        for(const auto& row:topology_entities_)if(row.at("ref").value("body",std::string())==p.owner&&row.at("kind")==kind(p.kind)&&row.at("id")==p.topology_id){auto ref=row.at("ref");ref["kind"]=kind(p.kind);return ref;}return {};
    };
    auto select=[&](const engine::PickRef& hit){auto ref=reference(hit);if(!ref.is_object())return;
        bool additive=io.KeyCtrl||io.KeyShift||(m.open&&m.command!="feature.hole");auto selected=additive?engine_->session().selection:J::array();
        auto same=[&](const auto& p){return p.value("kind",std::string())==kind(hit.kind)&&p.value("body",std::string())==hit.owner&&p.value("name",std::string())==ref.value("name",std::string());};
        auto it=std::find_if(selected.begin(),selected.end(),same);if(it==selected.end())selected.push_back(ref);else selected.erase(it);engine_->execute("selection.set",{{"refs",selected}});
        s.selected_profiles.clear();s.selected_profile_sketch.clear();if(hit.kind==engine::PickKind::Sketch)s.selected_profile_sketch=hit.owner;
        if(m.open&&m.command!="feature.hole"&&hit.kind==engine::PickKind::Edge){auto edge=std::find_if(m.edges.begin(),m.edges.end(),[&](const auto& p){return p.at("name")==ref.at("name")&&p.at("body")==ref.at("body");});if(edge==m.edges.end())m.edges.push_back(ref);else m.edges.erase(edge);}
        if(m.open&&m.command=="feature.hole"&&hit.kind==engine::PickKind::Face){m.face=ref;auto eye=s.camera.eye(),forward=s.camera.forward(),right=s.camera.right(),up=s.camera.up();double sx=(io.MousePos.x-v.x-v.width/2)/(v.height*1.2071067811865475),sy=-(io.MousePos.y-v.y-v.height/2)/(v.height*1.2071067811865475);kernel::Vec3 ray{forward.x+sx*right.x+sy*up.x,forward.y+sx*right.y+sy*up.y,forward.z+sx*right.z+sy*up.z};auto p=engine::face_position(context,ref,eye,ray);if(p){m.position=*p.value;m.references=J::array();}else m.error=p.error;}
        if(s.model.open&&!s.model.revolve&&hit.kind==engine::PickKind::Face){s.model.face=ref;s.model.sketch.clear();s.model.profiles.clear();}
    };
    int px=int(io.MousePos.x-v.x),py=int(io.MousePos.y-v.y);s.hovered_entity=hovered&&!s.sketch_mode&&!m.dragging?pick_buffer_.at(px,py):std::nullopt;
    if(hovered&&!s.sketch_mode&&!m.dragging&&s.navigation_mode<0&&ImGui::IsMouseClicked(0)){pick_pending_=true;pick_other_=false;pick_time_=ImGui::GetTime();pick_x_=io.MousePos.x;pick_y_=io.MousePos.y;}
    if(pick_pending_&&ImGui::IsMouseDown(0)&&std::hypot(io.MousePos.x-pick_x_,io.MousePos.y-pick_y_)<t.selection.edge_hit*z&&ImGui::GetTime()-pick_time_>=t.selection.hold_seconds){pick_candidates_=pick_buffer_.all(int(pick_x_-v.x),int(pick_y_-v.y));if(!pick_candidates_.empty()){ImGui::OpenPopup("Seleziona altro");pick_other_=true;pick_pending_=false;}}
    if(pick_pending_&&ImGui::IsMouseReleased(0)){if(hovered&&!(s.model_profile_under_pointer&&!m.open)&&std::hypot(io.MousePos.x-pick_x_,io.MousePos.y-pick_y_)<t.selection.edge_hit*z){if(s.hovered_entity)select(*s.hovered_entity);else if(!io.KeyCtrl&&!io.KeyShift&&!m.open)engine_->execute("selection.set",{{"refs",J::array()}});}pick_pending_=false;}
    if(ImGui::BeginPopup("Seleziona altro")){if(ImGui::IsKeyPressed(ImGuiKey_Escape)){ImGui::CloseCurrentPopup();pick_other_=false;}ImGui::TextUnformatted("SELEZIONA ALTRO");ImGui::Separator();for(size_t i=0;i<pick_candidates_.size();++i){const auto& p=pick_candidates_[i].ref;std::string label=(p.kind==engine::PickKind::Face?"Faccia ":p.kind==engine::PickKind::Edge?"Spigolo ":"Corpo ")+std::to_string(p.topology_id)+"##other"+std::to_string(i);if(ImGui::Selectable(label.c_str())){select(p);ImGui::CloseCurrentPopup();}if(ImGui::IsItemHovered())s.hovered_entity=p;}ImGui::EndPopup();}
    J selected=engine_->session().selection;if(m.open)selected=m.command=="feature.hole"?(m.face.is_object()?J::array({m.face}):J::array()):m.edges;
    auto matches=[&](const J& ref,const engine::PickRef& p){return ref.value("body",std::string())==p.owner&&((p.kind==engine::PickKind::Body&&ref.value("kind",std::string())=="body")||(reference(p).is_object()&&ref.value("name",std::string())==reference(p).value("name",std::string("!"))));};
    for(size_t index=0;index<surfaces_.size()&&index<surface_bodies_.size();++index){auto owner=surface_bodies_[index];const auto& geometry=surfaces_[index].geometry;
        for(const auto& tri:geometry.triangles){engine::PickRef p{engine::PickKind::Face,owner,tri.face_id};bool chosen=std::any_of(selected.begin(),selected.end(),[&](const auto& ref){if(ref.value("kind",std::string())=="body")return ref.value("body",std::string())==owner;return matches(ref,p);});bool hover=s.hovered_entity&&(s.hovered_entity==p||(s.hovered_entity->kind==engine::PickKind::Body&&s.hovered_entity->owner==owner));if(!chosen&&!hover)continue;
            auto flags=dl->Flags;dl->Flags&=~ImDrawListFlags_AntiAliasedFill;dl->AddTriangleFilled(project(tri.vertices[0]),project(tri.vertices[1]),project(tri.vertices[2]),color(chosen?t.model.selected:t.selection.face_hover));dl->Flags=flags;
        }
        std::set<std::string> selected_boundary,hover_boundary;
        for(const auto& row:topology_entities_)if(row.at("kind")=="face"&&row.at("ref").at("body")==owner){engine::PickRef face{engine::PickKind::Face,owner,row.at("id")};bool chosen=std::any_of(selected.begin(),selected.end(),[&](const auto& ref){return matches(ref,face);}),hover=s.hovered_entity==face;for(const auto& edge:row.at("boundary")){auto name=edge.at("name").get<std::string>();if(chosen)selected_boundary.insert(name);if(hover)hover_boundary.insert(name);}}
        for(const auto& edge:geometry.edges){engine::PickRef p{engine::PickKind::Edge,owner,edge.edge_id};bool chosen=std::any_of(selected.begin(),selected.end(),[&](const auto& ref){return matches(ref,p);}),hover=s.hovered_entity==p;auto ref=reference(p);auto name=ref.is_object()?ref.value("name",std::string()):"";bool boundary_selected=selected_boundary.contains(name),boundary_hover=hover_boundary.contains(name);if(!chosen&&!hover&&!boundary_selected&&!boundary_hover)continue;for(size_t i=1;i<edge.points.size();++i)dl->AddLine(project(edge.points[i-1]),project(edge.points[i]),color(chosen||boundary_selected?t.accent:t.selection.edge_hover),(chosen||hover?t.selection.edge_width:t.selection.face_width)*z);}
    }
    s.topology_labels=J::array();if(s.show_topology_labels){
        std::vector<const J*> labels;for(const auto& row:topology_entities_)if(std::find(surface_bodies_.begin(),surface_bodies_.end(),row.at("ref").at("body").get<std::string>())!=surface_bodies_.end())labels.push_back(&row);
        const float font=t.selection.label_size*z,line=font+t.spacing*z*.5f,label_width=t.selection.filter_width*z;int rows=std::max(1,int((v.height-2*t.padding*z)/line));if(int(labels.size())>rows)rows=std::max(1,int((v.height-t.cube.widget_size*z-2*t.padding*z)/line));
        for(size_t i=0;i<labels.size();++i){const auto& row=*labels[i];auto c=row.at("geometry").at("center").get<std::array<double,3>>();auto q=s.camera.project({c[0],c[1],c[2]},v);if(!q.visible)continue;auto name=row.at("name").get<std::string>();int column=int(i)/rows;float x=column==0?float(v.x)+t.padding*z:float(v.x+v.width)-label_width-t.padding*z;float y=float(v.y)+t.padding*z+float(int(i)%rows)*line+(column?t.cube.widget_size*z:0);ImVec2 position{x,y},end{x+label_width,y+line};dl->AddLine({float(q.x),float(q.y)},{x+label_width*.5f,y+line*.5f},color(t.border),z);dl->AddRectFilled(position,end,color(t.panel));dl->AddText(ImGui::GetFont(),font,position,color(t.text),name.c_str());dl->AddCircleFilled({float(q.x),float(q.y)},t.sketch.point_radius*z,color(t.accent));s.topology_labels.push_back(name);}
    }
    if(!m.open)return;
    auto params=ui::surface_params(s);auto tool_revision=document_revision+params.dump()+m.feature_id;
    if(tool_revision!=surface_tool_revision_){surface_tool_revision_=tool_revision;surface_preview_=engine::surface_preview(engine_->session().document,m.feature_id,m.command,params);m.preview_error=surface_preview_.error;
        if(m.command=="feature.hole"&&m.references.empty()&&surface_preview_.params.is_object()&&surface_preview_.params.contains("references")){m.references=surface_preview_.params.at("references");for(int i=0;i<2&&i<int(m.references.size());++i)copy(m.offsets[i],m.references[i].at("distance").get<std::string>());}}
    if(!surface_preview_.surface)return;
    if(m.command=="feature.hole"){auto flags=dl->Flags;dl->Flags&=~ImDrawListFlags_AntiAliasedFill;for(const auto& triangle:surface_preview_.cutter.triangles)dl->AddTriangleFilled(project(triangle.vertices[0]),project(triangle.vertices[1]),project(triangle.vertices[2]),color(t.model.cut));dl->Flags=flags;}

    auto a=project(surface_preview_.start),b=project(surface_preview_.handle);m.handle_x=b.x;m.handle_y=b.y;auto unit=surface_preview_.handle;unit.x+=surface_preview_.normal.x;unit.y+=surface_preview_.normal.y;unit.z+=surface_preview_.normal.z;auto u=project(unit);
    dl->AddLine(a,b,color(t.accent),t.sketch.line_width*z);double arrow_dx=b.x-a.x,arrow_dy=b.y-a.y,arrow_length=std::hypot(arrow_dx,arrow_dy);float nx=arrow_length>1?float(arrow_dx/arrow_length):0,ny=arrow_length>1?float(arrow_dy/arrow_length):1;ImVec2 left{b.x-nx*t.model.arrow_length*z-ny*t.model.arrow_width*z,b.y-ny*t.model.arrow_length*z+nx*t.model.arrow_width*z},right{b.x-nx*t.model.arrow_length*z+ny*t.model.arrow_width*z,b.y-ny*t.model.arrow_length*z-nx*t.model.arrow_width*z};dl->AddTriangleFilled(b,left,right,color(t.accent));ImVec2 border[3]={b,left,right};dl->AddPolyline(border,3,color(t.panel),ImDrawFlags_Closed,t.sketch.line_width*z);
    bool through=m.command=="feature.hole"&&m.through;
    if(hovered&&!through&&!m.dragging&&std::hypot(io.MousePos.x-b.x,io.MousePos.y-b.y)<t.model.handle_hit*z&&ImGui::IsMouseClicked(0)){m.dragging=true;m.drag_dimension=-1;m.drag_x=io.MousePos.x;m.drag_y=io.MousePos.y;m.drag_px=u.x-b.x;m.drag_py=u.y-b.y;m.drag_amount=surface_preview_.amount;pick_pending_=false;}
    for(size_t i=0;i<surface_preview_.dimensions.size()&&i<2;++i){const auto& d=surface_preview_.dimensions[i];auto from=project(d.from),to=project(d.to);dl->AddLine(from,to,color(t.accent),t.sketch.line_width*z);double length=std::hypot(to.x-from.x,to.y-from.y);float dx=length>1?float((to.x-from.x)/length):1,dy=length>1?float((to.y-from.y)/length):0;ImVec2 q{(from.x+to.x)/2-dy*t.sketch.dimension_offset*z,(from.y+to.y)/2+dx*t.sketch.dimension_offset*z};std::string label=m.offsets[i].data();auto size=ImGui::CalcTextSize(label.c_str());q.x=std::min(q.x,to.x-size.x-t.sketch.dimension_offset*z);dl->AddRectFilled({q.x-t.spacing*z,q.y-t.spacing*z},{q.x+size.x+t.spacing*z,q.y+size.y+t.spacing*z},color(t.panel));dl->AddText(q,color(t.text),label.c_str());m.fields["dimension"+std::to_string(i)]={q.x,q.y,size.x,size.y};
        if(hovered&&!m.dragging&&inside(q.x-t.spacing*z,q.y-t.spacing*z,size.x+2*t.spacing*z,size.y+2*t.spacing*z)&&ImGui::IsMouseClicked(0)){m.dragging=true;m.drag_dimension=int(i);m.drag_x=io.MousePos.x;m.drag_y=io.MousePos.y;auto p=d.to;p.x+=d.direction.x;p.y+=d.direction.y;p.z+=d.direction.z;auto one=project(p);m.drag_px=one.x-to.x;m.drag_py=one.y-to.y;m.drag_amount=d.value;pick_pending_=false;}}
    if(m.dragging){if(ImGui::IsMouseDown(0)){double value=std::max(.001,engine::model_drag_value(m.drag_amount,io.MousePos.x-m.drag_x,io.MousePos.y-m.drag_y,m.drag_px,m.drag_py,false));auto& field=m.drag_dimension>=0?m.offsets[m.drag_dimension]:m.command=="feature.hole"?m.depth:m.value;std::snprintf(field.data(),field.size(),"%.3f mm",value);}else m.dragging=false;}
    m.value_x=std::clamp(b.x+t.model.value_gap*z,float(v.x)+t.padding*z,float(v.x+v.width)-t.model.field_width*z-t.padding*z);m.value_y=std::clamp(b.y+t.model.value_gap*z,float(v.y)+t.padding*z,float(v.y+v.height)-t.model.field_height*z-t.padding*z);
    if(m.value_x+t.model.field_width*z>panel.x&&m.value_y>panel.y&&m.value_y<panel.y+panel.height)m.value_x=panel.x-t.model.field_width*z-t.model.value_gap*z;
    ImGui::SetCursorScreenPos({m.value_x,m.value_y});ImGui::SetNextItemWidth(t.model.field_width*z);ImGui::BeginDisabled(through);auto& field=m.command=="feature.hole"?m.depth:m.value;if(through){std::array<char,16> text{};copy(text,"Passante");ImGui::InputText("##surface_value",text.data(),text.size(),ImGuiInputTextFlags_ReadOnly);}else ImGui::InputText("##surface_value",field.data(),field.size(),ImGuiInputTextFlags_AutoSelectAll);ImGui::EndDisabled();
}
void Shell::draw_surface_panel(const ui::Theme& t){
    auto& s=state_;auto& m=s.surface;const float z=float(scale_);
    if(s.selection_filters_open){ImGui::OpenPopup("Filtri di selezione");s.selection_filters_open=false;}
    if(ImGui::BeginPopup("Filtri di selezione")){ImGui::TextUnformatted("FILTRI DI SELEZIONE");ImGui::Checkbox("Facce",&s.pick_filter.faces);ImGui::Checkbox("Spigoli",&s.pick_filter.edges);ImGui::Checkbox("Corpi",&s.pick_filter.bodies);ImGui::Checkbox("Schizzi",&s.pick_filter.sketches);ImGui::EndPopup();}
    if(!m.open||!engine_)return;auto r=layout_.last().contextual;if(!r.visible())return;
    ImGui::SetNextWindowPos({r.x,r.y});ImGui::SetNextWindowSize({r.width,r.height});ImGui::Begin("##surface_panel",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings);
    ImGui::TextUnformatted(m.command=="feature.fillet"?"RACCORDO":m.command=="feature.chamfer"?"SMUSSO":"FORO");ImGui::Separator();
    ImGui::BeginChild("surface_fields",{0,-t.model.footer*z},false);
    auto field=[&](const char* label,const char* key,auto& buffer){ImGui::AlignTextToFramePadding();ImGui::TextUnformatted(label);ImGui::SameLine(t.model.label_width*z);ImGui::SetNextItemWidth(-t.padding*z);ImGui::InputText((std::string("##")+key).c_str(),buffer.data(),buffer.size(),ImGuiInputTextFlags_AutoSelectAll);auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();m.fields[key]={a.x,a.y,b.x-a.x,b.y-a.y};};
    if(m.command=="feature.hole"){
        ImGui::TextUnformatted("Posizionamento: su faccia");ImGui::Text("Faccia: %s",m.face.is_object()?"1 selezionata":"Seleziona");
        for(int i=0;i<2;++i){std::string label="Riferimento "+std::to_string(i+1),key="offset"+std::to_string(i);field(label.c_str(),key.c_str(),m.offsets[i]);if(i<int(m.references.size())&&ImGui::IsItemHovered())ImGui::SetTooltip("%s",m.references[i].at("edge").at("name").get<std::string>().c_str());}
        ImGui::Separator();field("Diametro","diameter",m.diameter);ImGui::TextUnformatted("Estensione");ImGui::SameLine(t.model.label_width*z);int extent=m.through?1:0;ImGui::SetNextItemWidth(-t.padding*z);if(ImGui::Combo("##extent",&extent,"Distanza\0Passante\0"))m.through=extent==1;ImGui::BeginDisabled(m.through);field("Profondità","depth",m.depth);ImGui::EndDisabled();
    }else{
        ImGui::Text("%zu Spigoli",m.edges.size());ImGui::BeginChild("edge_list",{0,t.model.row*4*z},true);for(size_t i=0;i<m.edges.size();++i){auto label="Spigolo "+std::to_string(i+1)+"##edge"+std::to_string(i);if(ImGui::Selectable(label.c_str(),true)){m.edges.erase(m.edges.begin()+i);break;}if(ImGui::IsItemHovered())ImGui::SetTooltip("%s",m.edges[i].at("name").get<std::string>().c_str());}ImGui::EndChild();
        ImGui::TextUnformatted(m.command=="feature.fillet"?"Tipo raggio: Costante":"Tipo: Distanza uguale");field(m.command=="feature.fillet"?"Raggio":"Distanza","amount",m.value);ImGui::TextUnformatted("Spigoli: seleziona nel viewport");
    }
    if(!m.preview_error.empty())ImGui::TextWrapped("%s",m.preview_error.c_str());if(!m.error.empty())ImGui::TextWrapped("%s",m.error.c_str());ImGui::EndChild();ImGui::Separator();
    ImGui::BeginDisabled(!m.preview_error.empty());if(ImGui::Button("OK"))ui::submit_surface(s,*engine_);ImGui::EndDisabled();auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();m.fields["ok"]={a.x,a.y,b.x-a.x,b.y-a.y};ImGui::SameLine();if(ImGui::Button("Annulla")||ImGui::IsKeyPressed(ImGuiKey_Escape)){m.open=false;m.dragging=false;}ImGui::End();
}
}
