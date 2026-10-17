#include "shell.hpp"
#include <imgui.h>
#include <algorithm>
#include <cmath>
namespace sagomacad {
namespace {
ImU32 color(ui::Color c){return ImGui::ColorConvertFloat4ToU32({c.r,c.g,c.b,c.a});}
}
void Shell::draw_timeline(const ui::Theme& t){
    if(!engine_)return;
    auto& s=state_;const float z=float(scale_);auto& io=ImGui::GetIO();auto* dl=ImGui::GetWindowDrawList();const auto q=ImGui::GetCursorScreenPos();const auto& tokens=t.timeline;
    const auto& document=engine_->session().document;const auto count=document.timeline.size();
    size_t visible_marker=document.rollback;std::string editing=s.surface.open?s.surface.feature_id:s.model.open?s.model.feature_id:s.primitive_editor_open?s.primitive_feature_id:s.sketch_mode?s.sketch_view_id:"";
    if(!editing.empty()){auto at=std::find(document.timeline.begin(),document.timeline.end(),editing);if(at!=document.timeline.end())visible_marker=std::size_t(at-document.timeline.begin())+(s.sketch_mode?1:0);}
    auto rollback=[&](size_t index){auto r=engine_->execute("timeline.rollback",{{"index",index}},"marker");if(!r.ok)s.status_message=r.error;};
    if(s.timeline_playing){s.timeline_elapsed+=io.DeltaTime;if(s.timeline_elapsed>=tokens.play_interval){s.timeline_elapsed=0;if(document.rollback<count)rollback(document.rollback+1);else s.timeline_playing=false;}}
    const char* controls[]={"skip-back","step-back",s.timeline_playing?"pause":"play","step-forward","skip-forward"};
    for(int i=0;i<5;++i){
        const ImVec2 p{q.x+i*(tokens.control+tokens.gap)*z,q.y+tokens.row_top*z};ImGui::SetCursorScreenPos(p);ImGui::PushID(i);
        if(ImGui::InvisibleButton("timeline_control",{tokens.control*z,tokens.item*z})){
            s.timeline_playing=i==2?!s.timeline_playing:false;
            if(i==0)rollback(0);if(i==1)rollback(document.rollback?document.rollback-1:0);if(i==2&&document.rollback==count)rollback(0);if(i==3)rollback(std::min(count,document.rollback+1));if(i==4)rollback(count);
        }
        auto texture=icon_texture(controls[i]);if(texture)dl->AddImage((ImTextureID)(intptr_t)texture,{p.x+(tokens.control-tokens.icon)*z/2,p.y+(tokens.item-tokens.icon)*z/2},{p.x+(tokens.control+tokens.icon)*z/2,p.y+(tokens.item+tokens.icon)*z/2},{0,0},{1,1},color(t.muted));
        if(ImGui::IsItemHovered()){const char* tips[]={"Torna all'inizio","Passo indietro","Riproduci / pausa","Passo avanti","Vai alla fine"};ImGui::SetTooltip("%s",tips[i]);}
        ImGui::PopID();
    }
    const float start=q.x+5*(tokens.control+tokens.gap)*z+tokens.gap*z,top=q.y+tokens.row_top*z,step=(tokens.item+tokens.gap)*z;
    s.timeline_items.clear();
    const auto snapshot=regenerator_.evaluate(document);
    std::string action,pending;
    for(size_t i=0;i<count;++i){const auto& id=document.timeline[i];auto f=document.features.find(id);if(f==document.features.end())continue;
        const auto& feature=f->second;const auto row=std::find_if(snapshot.features.begin(),snapshot.features.end(),[&](const auto& r){return r.id==id;});
        const bool failed=row!=snapshot.features.end()&&!row->error.empty(),inactive=i>=visible_marker||feature.suppressed;
        const ImVec2 p{start+i*step,top};s.timeline_items.push_back({id,p.x,p.y,tokens.item*z,tokens.item*z});
        ImGui::SetCursorScreenPos(p);ImGui::PushID(id.c_str());ImGui::InvisibleButton("feature",{tokens.item*z,tokens.item*z});
        const bool hovered=ImGui::IsItemHovered();
        if(hovered&&ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))open_feature_editor(id);
        if(hovered)ImGui::SetTooltip("%s\n%s",feature.name.c_str(),failed?row->error.c_str():feature.suppressed?"Soppressa":inactive?"Dopo il marker di ritorno":"Doppio clic per modificare");
        if(ImGui::BeginPopupContextItem("feature_context")){
            if(ImGui::MenuItem("Modifica feature"))open_feature_editor(id);
            if(ImGui::MenuItem(feature.suppressed?"Ripristina":"Sopprimi")){action="feature.suppress";pending=id;}
            if(ImGui::MenuItem("Elimina")){action="feature.delete";pending=id;}
            ImGui::EndPopup();
        }
        auto tint=failed?t.error:inactive?t.muted:t.text;if(inactive)tint.a=.35f;
        if(hovered)dl->AddRectFilled(p,{p.x+tokens.item*z,p.y+tokens.item*z},color(t.panel_alt),t.rounding*z);
        const char* glyph=feature.type=="sketch"?"pen-tool":feature.type=="feature.extrude"?"move-3d":feature.type=="feature.revolve"?"rotate-3d":feature.type=="feature.fillet"?"blend":feature.type=="feature.chamfer"?"diamond":feature.type=="feature.hole"?"circle-dot":feature.type=="primitive.box"?"box":feature.type=="primitive.cylinder"?"circle-dot":"circle";
        auto texture=icon_texture(glyph);const float inset=(tokens.item-tokens.icon)*z/2;
        if(texture)dl->AddImage((ImTextureID)(intptr_t)texture,{p.x+inset,p.y+inset},{p.x+inset+tokens.icon*z,p.y+inset+tokens.icon*z},{0,0},{1,1},color(tint));
        if(feature.suppressed)dl->AddLine(p,{p.x+tokens.item*z,p.y+tokens.item*z},color(t.muted),z);
        ImGui::PopID();
    }
    const float marker=start+visible_marker*step-tokens.gap*z/2;
    s.timeline_marker_x=marker;s.timeline_marker_y=top;
    ImGui::SetCursorScreenPos({marker-tokens.marker_hit*z/2,top});ImGui::InvisibleButton("rollback_marker",{tokens.marker_hit*z,tokens.marker_height*z});
    if(ImGui::IsItemActivated())s.timeline_dragging=true;
    if(s.timeline_dragging){if(ImGui::IsMouseDown(0)){s.timeline_playing=false;const auto target=std::clamp(std::llround((io.MousePos.x-start+tokens.gap*z/2)/step),0LL,static_cast<long long>(count));if(std::size_t(target)!=document.rollback)rollback(std::size_t(target));}else s.timeline_dragging=false;}
    dl->AddLine({marker,top},{marker,top+tokens.marker_height*z},color(t.accent),tokens.marker_width*z);
    dl->AddTriangleFilled({marker-tokens.marker_hit*z/2,top},{marker+tokens.marker_hit*z/2,top},{marker,top+tokens.marker_hit*z/2},color(t.accent));
    if(ImGui::IsItemHovered())ImGui::SetTooltip("Trascina il marker di ritorno (%zu / %zu)",document.rollback,count);
    if(!action.empty()){nlohmann::json p={{"feature_id",pending}};if(action=="feature.suppress")p["suppressed"]=!document.features.at(pending).suppressed;const auto result=engine_->execute(action,p);if(!result.ok)s.status_message=result.error;}
}
}
