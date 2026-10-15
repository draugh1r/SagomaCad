#include "shell.hpp"
#include "ui/sketch_view.hpp"
#include "engine/tools/sketch_overlay.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numbers>

namespace sagomacad {
namespace {
ImU32 color(ui::Color c){return ImGui::ColorConvertFloat4ToU32({c.r,c.g,c.b,c.a});}
ImVec4 rgba(ui::Color c){return {c.r,c.g,c.b,c.a};}
double distance(ImVec2 a,ImVec2 b){return std::hypot(a.x-b.x,a.y-b.y);}
double segment_distance(ImVec2 p,ImVec2 a,ImVec2 b){double dx=b.x-a.x,dy=b.y-a.y,den=dx*dx+dy*dy,t=den>0?std::clamp(((p.x-a.x)*dx+(p.y-a.y)*dy)/den,0.,1.):0;return std::hypot(p.x-a.x-t*dx,p.y-a.y-t*dy);}
}
void Shell::sync_sketch(){
    if(!engine_)return;auto& session=engine_->session();
    if(!session.active_sketch.empty()&&!session.document.features.contains(session.active_sketch))session.active_sketch.clear();
    if(state_.sketch_view_id!=session.active_sketch){
        state_.sketch_mode=!session.active_sketch.empty();state_.sketch_view_id=session.active_sketch;state_.sketch_selection.clear();state_.sketch_tool.begin("select");state_.sketch_error.clear();
        if(state_.sketch_mode){state_.sketch_plane_picker=false;state_.navigation_mode=-1;state_.primitive_create_open=false;state_.primitive_editor_open=false;
            const auto& f=session.document.features.at(session.active_sketch);const auto plane=f.data.value("plane",std::string("XY"));auto destination=ui::sketch_camera(state_.camera,plane);
            state_.view_transition.begin(state_.camera,destination,ui::theme(state_.theme=="light").cube.transition_duration);
        }
    }
    const auto revision=doc::to_json(session.document).dump();
    if(revision!=sketch_revision_){sketch_revision_=revision;sketch_solutions_.clear();
        state_.sketch_error.clear();for(const auto& [id,f]:session.document.features)if(f.type=="sketch"&&!f.suppressed){
        try{sketch_solutions_[id]=solver::solve(session.document,doc::read_sketch(f.data));}catch(const std::exception& e){solver::SketchResult r;r.state="error";r.error=e.what();sketch_solutions_[id]=r;}
    }}
}
void Shell::draw_sketch_canvas(const ui::Theme& t,bool hovered){
    if(!engine_)return;auto& s=state_;auto& tool=s.sketch_tool;const float z=float(scale_);auto* dl=ImGui::GetWindowDrawList();auto& io=ImGui::GetIO();const auto v=current_viewport_;
    auto project=[&](std::string_view plane,engine::SketchXY p){auto q=s.camera.project(ui::sketch_world(plane,p),v);return ImVec2{float(q.x),float(q.y)};};
    if(s.sketch_plane_picker){
        const double extent=t.sketch.plane_extent;const auto selected=hovered?ui::pick_sketch_plane(s.camera,v,io.MousePos.x,io.MousePos.y,extent):"";
        for(auto plane:{"YZ","XZ","XY"}){ImVec2 quad[4]={project(plane,{0,0}),project(plane,{extent,0}),project(plane,{extent,extent}),project(plane,{0,extent})};dl->AddConvexPolyFilled(quad,4,color(selected==plane?t.sketch.plane_hover:t.sketch.plane));dl->AddPolyline(quad,4,color(t.border),ImDrawFlags_Closed,t.sketch.line_width*z);auto label=project(plane,{extent*.65,extent*.65});dl->AddText(label,color(t.text),plane);}
        if(hovered){auto pos=io.MousePos;pos.x+=t.padding*z;pos.y+=t.padding*z;auto size=ImGui::CalcTextSize(t.sketch.select_plane);dl->AddRectFilled(pos,{pos.x+size.x+t.padding*z,pos.y+size.y+t.spacing*z},color(t.panel));dl->AddText(pos,color(t.text),t.sketch.select_plane);
            if(ImGui::IsMouseClicked(ImGuiMouseButton_Left)&&!selected.empty()){auto r=engine_->execute("sketch.create",{{"plane",selected}});if(!r.ok)s.sketch_error=r.error;else{sync_sketch();}}
        }
        if(ImGui::IsKeyPressed(ImGuiKey_Escape))s.sketch_plane_picker=false;
    }
    std::string plane="XY";const solver::SketchResult* active=nullptr;
    if(s.sketch_mode&&sketch_solutions_.contains(s.sketch_view_id)){active=&sketch_solutions_.at(s.sketch_view_id);plane=active->sketch.plane;}
    const float palette_left=float(v.x+v.width)-t.sketch.palette_width*z-t.padding*z;
    const bool over_palette=s.sketch_mode&&io.MousePos.x>=palette_left&&io.MousePos.y>=v.y+t.sketch.palette_y*z;
    hovered=hovered&&!over_palette;
    if(s.sketch_mode&&s.sketch_grid){
        double extent=s.camera.distance;
        double raw=s.camera.distance/(v.height*1.2071067811865475)*t.sketch.grid_pixel_step*z;
        double base=std::pow(10.,std::floor(std::log10(std::max(raw,1e-6))));
        double step=base*(raw/base<=1?1:raw/base<=2?2:raw/base<=5?5:10);int count=std::min(150,int(extent/step));
        for(int i=-count;i<=count;++i){double a=i*step;auto p=project(plane,{a,-extent}),q=project(plane,{a,extent}),r=project(plane,{-extent,a}),u=project(plane,{extent,a});
            dl->AddLine(p,q,color(i==0?t.axis_y:i%5==0?t.grid_major:t.grid_minor),i==0?t.sketch.line_width*z:z);dl->AddLine(r,u,color(i==0?t.axis_x:i%5==0?t.grid_major:t.grid_minor),i==0?t.sketch.line_width*z:z);
            if(i%5==0&&i!=0){char label[32];std::snprintf(label,sizeof(label),"%g",a);auto x=project(plane,{a,0}),y=project(plane,{0,a});dl->AddText({x.x+t.spacing*z,x.y+t.spacing*z},color(t.muted),label);dl->AddText({y.x+t.spacing*z,y.y+t.spacing*z},color(t.muted),label);}
        }
    }
    struct DimensionHit {std::string id,expression,kind;ImVec2 a,b;};std::vector<DimensionHit> dimension_hits;
    auto draw=[&](const solver::SketchResult& result,bool preview){
        const auto overlay=engine::sketch_overlay(result);const auto& pl=result.sketch.plane;
        if(s.sketch_profiles)for(const auto& profile:overlay.profiles){std::vector<ImVec2> poly;for(auto p:profile)poly.push_back(project(pl,p));if(poly.size()>=3)dl->AddConcavePolyFilled(poly.data(),int(poly.size()),color(t.sketch.profile));}
        for(const auto& stroke:overlay.strokes){if(stroke.construction&&!s.sketch_construction)continue;auto c=result.state=="conflict"||result.state=="error"?t.error:stroke.fixed?(s.theme=="light"?t.sketch.constrained:t.text):t.sketch.free;
            if(preview||std::find(s.sketch_selection.begin(),s.sketch_selection.end(),stroke.id)!=s.sketch_selection.end())c=t.accent;
            for(size_t i=1;i<stroke.points.size();++i){if(stroke.construction&&i%2==0)continue;dl->AddLine(project(pl,stroke.points[i-1]),project(pl,stroke.points[i]),color(c),t.sketch.line_width*z);}
        }
        if(s.sketch_points)for(const auto& p:result.sketch.points){bool fixed=result.fixed_points.contains(p.id)&&result.fixed_points.at(p.id);auto c=result.state=="conflict"?t.error:fixed?(s.theme=="light"?t.sketch.constrained:t.text):t.sketch.free;dl->AddCircleFilled(project(pl,{p.x,p.y}),t.sketch.point_radius*z,color(c));}
        if(!preview&&s.sketch_mode&&s.sketch_dimensions)for(size_t i=0;i<overlay.dimensions.size();++i){const auto& label=overlay.dimensions[i];auto q=project(pl,label.position);auto text=label.text;auto size=ImGui::CalcTextSize(text.c_str());q.x-=size.x/2;q.y-=t.sketch.dimension_offset*z;
            if(label.vertical){q.x+=t.sketch.dimension_offset*z+size.x/2;q.y+=t.sketch.dimension_offset*z;}
            ImVec2 end{q.x+size.x+t.spacing*z,q.y+size.y+t.spacing*z};dl->AddRectFilled({q.x-t.spacing*z/2,q.y-t.spacing*z/2},end,color(t.panel));dl->AddText(q,color(result.state=="conflict"?t.error:t.text),text.c_str());dimension_hits.push_back({label.id,label.text,label.kind,{q.x-t.spacing*z/2,q.y-t.spacing*z/2},end});
        }
        if(!preview&&s.sketch_mode&&s.sketch_constraints)for(const auto& label:overlay.constraints){auto q=project(pl,label.position);dl->AddText(ImGui::GetFont(),t.sketch.constraint_size*z,{q.x+t.spacing*z,q.y+t.spacing*z},color(t.muted),label.text.c_str());}
    };
    for(const auto& [id,result]:sketch_solutions_)if(!s.sketch_mode||id==s.sketch_view_id)draw(result,false);
    if(!active||!s.sketch_mode)return;
    if(!active->error.empty())s.sketch_error=active->error;
    engine::SketchXY cursor;bool cursor_ok=ui::sketch_cursor(s.camera,v,plane,io.MousePos.x,io.MousePos.y,cursor);
    if(cursor_ok&&s.sketch_snap){double threshold=t.sketch.hit_radius*z*s.camera.distance/(v.height*1.2071067811865475);if(std::hypot(cursor.x,cursor.y)<threshold)cursor={};else for(const auto& p:active->sketch.points)if(std::hypot(cursor.x-p.x,cursor.y-p.y)<threshold){cursor={p.x,p.y};break;}}
    if(hovered&&cursor_ok)tool.move(cursor.x,cursor.y);
    auto new_dimension=[&](){
        s.sketch_dimension_id.clear();s.sketch_dimension_kind="distance";s.sketch_dimension_expression.fill(0);
        bool first_line=false,second_line=false;
        for(const auto& e:active->sketch.entities){
            if(!s.sketch_selection.empty()&&e.id==s.sketch_selection[0]){s.sketch_dimension_kind=e.kind=="circle"?"diameter":e.kind=="arc"?"radius":"length";first_line=e.kind=="line";}
            if(s.sketch_selection.size()>1&&e.id==s.sketch_selection[1])second_line=e.kind=="line";
        }
        if(first_line&&second_line)s.sketch_dimension_kind="angle";
        if(!s.sketch_selection.empty()&&(s.sketch_dimension_kind!="distance"||s.sketch_selection.size()==2))s.sketch_dimension_open=true;
    };
    if(!io.WantTextInput&&!s.palette_open&&!s.sketch_dimension_open&&!io.KeyCtrl){
        if(ImGui::IsKeyPressed(ImGuiKey_L))tool.begin("line");if(ImGui::IsKeyPressed(ImGuiKey_R))tool.begin("rect");if(ImGui::IsKeyPressed(ImGuiKey_C))tool.begin("circle");if(ImGui::IsKeyPressed(ImGuiKey_D)){tool.begin("dimension");new_dimension();}
    }
    if(ImGui::IsKeyPressed(ImGuiKey_Escape)){tool.escape();s.sketch_dimension_open=false;s.sketch_constraint_open=false;s.sketch_field_focus=false;}
    bool dimension_click=false;
    if(hovered&&!tool.started)for(const auto& hit:dimension_hits)if(io.MousePos.x>=hit.a.x&&io.MousePos.x<=hit.b.x&&io.MousePos.y>=hit.a.y&&io.MousePos.y<=hit.b.y){
        dimension_click=true;if(ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)){s.sketch_dimension_id=hit.id;s.sketch_dimension_kind=hit.kind;std::snprintf(s.sketch_dimension_expression.data(),s.sketch_dimension_expression.size(),"%s",hit.expression.c_str());s.sketch_dimension_open=true;}
    }
    if(hovered&&cursor_ok&&!dimension_click&&ImGui::IsMouseClicked(ImGuiMouseButton_Left)){
        if(tool.kind!="select"&&tool.kind!="dimension"){
            if(!tool.started){tool.click(cursor.x,cursor.y);s.sketch_fields={};s.sketch_field_focus=true;}
            else{tool.move(cursor.x,cursor.y);auto r=tool.confirm(*engine_);if(r.ok&&r.value.value("preview",false)){std::snprintf(s.sketch_fields[0].data(),s.sketch_fields[0].size(),"%s",tool.fields[0].c_str());tool.active_field=1;s.sketch_field_focus=true;}}
        }else{
            std::string selected;double nearest=t.sketch.hit_radius*z;
            for(const auto& p:active->sketch.points){double d=distance(io.MousePos,project(plane,{p.x,p.y}));if(d<nearest){nearest=d;selected=p.id;}}
            if(!selected.empty()&&tool.kind=="select"){s.sketch_drag_point=selected;}
            if(selected.empty())for(const auto& stroke:engine::sketch_overlay(*active).strokes)for(size_t i=1;i<stroke.points.size();++i){double d=segment_distance(io.MousePos,project(plane,stroke.points[i-1]),project(plane,stroke.points[i]));if(d<nearest){nearest=d;selected=stroke.id;}}
            if(!io.KeyShift)s.sketch_selection.clear();if(!selected.empty())s.sketch_selection.push_back(selected);
            if(tool.kind=="dimension"&&!selected.empty()){
                new_dimension();
            }
        }
    }
    if(!s.sketch_drag_point.empty()){
        if(ImGui::IsMouseDragging(ImGuiMouseButton_Left)&&cursor_ok){auto r=engine_->execute("sketch.point.move",{{"point",s.sketch_drag_point},{"x",cursor.x},{"y",cursor.y}},"drag:"+s.sketch_drag_point);if(!r.ok)s.sketch_error=r.error;}
        if(!ImGui::IsMouseDown(ImGuiMouseButton_Left))s.sketch_drag_point.clear();
    }
    if(tool.started){
        engine::Engine preview;preview.session().document=engine_->session().document;preview.session().active_sketch=s.sketch_view_id;
        const auto command=tool.preview_command();
        auto result=preview.execute(command["command"].get<std::string>(),command["params"]);
        if(result.ok){auto solved=solver::solve(preview.session().document,doc::read_sketch(preview.session().document.features.at(s.sketch_view_id).data));draw(solved,true);}
    }
    if(hovered&&!io.WantTextInput){auto pos=io.MousePos;auto hint=tool.hint();auto size=ImGui::CalcTextSize(hint.c_str());pos.x+=t.sketch.dimension_offset*z;pos.y+=t.sketch.dimension_offset*z;
        if(pos.x+size.x>palette_left-t.padding*z)pos.x=std::max(float(v.x)+t.padding*z,io.MousePos.x-size.x-t.sketch.dimension_offset*z);
        dl->AddText(pos,color(t.muted),hint.c_str());}
}
void Shell::draw_sketch_panels(const ui::Theme& t){
    auto& s=state_;if(!engine_||!s.sketch_mode||!sketch_solutions_.contains(s.sketch_view_id))return;
    const auto& result=sketch_solutions_.at(s.sketch_view_id);auto& tool=s.sketch_tool;const float z=float(scale_);const auto v=current_viewport_;auto* previous=ImGui::GetCurrentContext();(void)previous;
    ImGui::SetNextWindowPos({float(v.x+v.width)-t.sketch.palette_width*z-t.padding*z,float(v.y)+t.sketch.palette_y*z},ImGuiCond_Always);
    ImGui::SetNextWindowSize({t.sketch.palette_width*z,std::min(t.sketch.palette_height*z,float(v.height)-t.sketch.palette_y*z-t.padding*z)},ImGuiCond_Always);
    ImGui::Begin(t.sketch.palette,nullptr,ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoScrollbar);
    ImGui::BeginChild("##sketch_options",{0,-ImGui::GetFrameHeightWithSpacing()-t.padding*z-t.spacing*z});
    ImGui::TextColored(rgba(result.state=="conflict"||result.state=="error"?t.error:t.muted),"%s · %d GDL",result.state=="constrained"?"Completamente vincolato":result.state=="conflict"?"In conflitto":result.state=="error"?"Errore di valutazione":"Libero",result.degrees_of_freedom);
    if(ImGui::Button("Guarda")){auto dest=ui::sketch_camera(s.camera,result.sketch.plane);s.view_transition.begin(s.camera,dest,t.cube.transition_duration);}
    ImGui::Checkbox("Griglia",&s.sketch_grid);ImGui::Checkbox("Aggancio",&s.sketch_snap);ImGui::Checkbox("Profilo",&s.sketch_profiles);ImGui::Checkbox("Punti",&s.sketch_points);ImGui::Checkbox("Quote",&s.sketch_dimensions);ImGui::Checkbox("Vincoli",&s.sketch_constraints);ImGui::Checkbox("Geometrie di costruzione",&s.sketch_construction);
    if(!result.redundant.empty()){ImGui::TextWrapped("Vincoli ridondanti:");for(const auto& id:result.redundant)ImGui::TextUnformatted(id.c_str());}
    if(!s.sketch_error.empty()){ImGui::PushStyleColor(ImGuiCol_Text,rgba(t.error));ImGui::TextWrapped("%s",s.sketch_error.c_str());ImGui::PopStyleColor();}
    ImGui::EndChild();ImGui::Separator();ImGui::PushStyleColor(ImGuiCol_Button,rgba(t.sketch.finish));if(ImGui::Button(t.sketch.finish_label))invoke_command("sketch.finish");ImGui::PopStyleColor();ImGui::End();
    if(tool.started){
        auto projection=s.camera.project(ui::sketch_world(result.sketch.plane,{tool.cursor_x,tool.cursor_y}),v);
        float left=std::clamp(float(projection.x)+t.sketch.dimension_offset*z,float(v.x)+t.padding*z,float(v.x+v.width)-t.sketch.palette_width*z-t.sketch.field_width*z-t.padding*z*3);
        float top=std::clamp(float(projection.y)+t.sketch.dimension_offset*z,float(v.y)+t.padding*z,float(v.y+v.height)-t.sketch.field_height*z*5);
        ImGui::SetNextWindowPos({left,top},ImGuiCond_Always);ImGui::SetNextWindowSize({t.sketch.field_width*z+t.padding*z*2,0},ImGuiCond_Always);
        ImGui::Begin("##sketch_live_dimensions",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoMouseInputs);
        const char* labels[2]={tool.kind=="rect"?"Larghezza":tool.kind=="circle"?"Diametro":tool.kind=="arc"?"Raggio":"Lunghezza",tool.kind=="rect"?"Altezza":"Angolo"};
        int count=tool.kind=="circle"?1:2;bool confirm=false;int focus=s.sketch_field_focus?tool.active_field:-1;s.sketch_field_focus=false;
        const auto live=tool.live_values();
        for(int i=0;i<count;++i){ImGui::PushID(i);ImGui::TextColored(rgba(i==tool.active_field?t.accent:t.muted),"%s%s",labels[i],tool.locked[i]?"  ·":"");
            if(!tool.locked[i]) {
                std::snprintf(s.sketch_fields[i].data(),s.sketch_fields[i].size(),"%s",live[i].c_str());
                // ImGui retains the active text buffer. Reload live measurements
                // and select them so the first typed character replaces the value.
                if(auto* input=ImGui::GetInputTextState(ImGui::GetID("##measure")))input->ReloadUserBufAndSelectAll();
            }
            if(focus==i)ImGui::SetKeyboardFocusHere();ImGui::SetNextItemWidth(t.sketch.field_width*z);
            auto callback=[](ImGuiInputTextCallbackData* data)->int{auto* state=static_cast<ui::State*>(data->UserData);state->sketch_tool.tab();state->sketch_field_focus=true;return 0;};
            bool entered=ImGui::InputText("##measure",s.sketch_fields[i].data(),s.sketch_fields[i].size(),ImGuiInputTextFlags_EnterReturnsTrue|ImGuiInputTextFlags_CallbackCompletion|ImGuiInputTextFlags_AutoSelectAll,callback,&s);
            if(ImGui::IsItemEdited()){tool.fields[i]=s.sketch_fields[i].data();tool.locked[i]=!tool.fields[i].empty();}
            if(ImGui::IsItemActive()&&!s.sketch_field_focus)tool.active_field=i;
            if(entered)confirm=true;ImGui::PopID();
        }
        if(!tool.error.empty()){ImGui::TextColored(rgba(t.error),"%s",tool.error.c_str());}
        ImGui::End();if(confirm){auto r=tool.confirm(*engine_);if(r.ok&&r.value.value("preview",false)){std::snprintf(s.sketch_fields[0].data(),s.sketch_fields[0].size(),"%s",tool.fields[0].c_str());tool.active_field=1;s.sketch_field_focus=true;}if(!tool.error.empty())s.sketch_error=tool.error;}
    }
    if(s.sketch_dimension_open){
        ImGui::SetNextWindowSize({t.file_dialog_width*z,0},ImGuiCond_Appearing);ImGui::Begin("Quota schizzo",&s.sketch_dimension_open,ImGuiWindowFlags_AlwaysAutoResize);
        if(s.sketch_dimension_id.empty()) {
            const std::array<std::string,5> kinds={"length","distance","diameter","radius","angle"};
            int kind=int(std::find(kinds.begin(),kinds.end(),s.sketch_dimension_kind)-kinds.begin());
            if(ImGui::Combo("Tipo",&kind,"Lunghezza\0Distanza\0Diametro\0Raggio\0Angolo\0"))s.sketch_dimension_kind=kinds[kind];
        }else ImGui::Text("Tipo: %s",s.sketch_dimension_kind.c_str());
        bool enter=ImGui::InputText("Espressione",s.sketch_dimension_expression.data(),s.sketch_dimension_expression.size(),ImGuiInputTextFlags_EnterReturnsTrue);
        if(enter||ImGui::Button("OK")){nlohmann::json p={{"expression",s.sketch_dimension_expression.data()}};std::string id="sketch.dimension.set";if(s.sketch_dimension_id.empty()){id="sketch.dimension.add";p["kind"]=s.sketch_dimension_kind;p["refs"]=s.sketch_selection;}else p["dimension"]=s.sketch_dimension_id;
            auto r=engine_->execute(id,p);s.sketch_error=r.ok?r.value["solver"].value("error",std::string{}):r.error;if(r.ok)s.sketch_dimension_open=false;
        }ImGui::SameLine();if(ImGui::Button("Annulla"))s.sketch_dimension_open=false;
        if(!s.sketch_error.empty())ImGui::TextColored(rgba(t.error),"%s",s.sketch_error.c_str());ImGui::End();
    }
    if(s.sketch_constraint_open){
        ImGui::Begin("Vincolo schizzo",&s.sketch_constraint_open,ImGuiWindowFlags_AlwaysAutoResize);
        const char* kinds[]={"coincident","horizontal","vertical","parallel","perpendicular","tangent","equal","midpoint","fix"};
        ImGui::Combo("Tipo",&s.sketch_constraint_kind,"Coincidente\0Orizzontale\0Verticale\0Parallelo\0Perpendicolare\0Tangente\0Uguale\0Punto medio\0Fisso\0");ImGui::TextUnformatted("Seleziona le geometrie con Shift+clic.");
        for(const auto& id:s.sketch_selection)ImGui::TextUnformatted(id.c_str());
        if(ImGui::Button("Applica")){auto r=engine_->execute("sketch.constraint.add",{{"kind",kinds[s.sketch_constraint_kind]},{"refs",s.sketch_selection}});s.sketch_error=r.ok?r.value["solver"].value("error",std::string{}):r.error;}
        if(!s.sketch_error.empty())ImGui::TextColored(rgba(t.error),"%s",s.sketch_error.c_str());ImGui::End();
    }
}
}
