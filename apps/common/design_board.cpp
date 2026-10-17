#include "shell.hpp"
#include "ui/frame.hpp"
#include "ui/toolbar_catalog.hpp"
#include <SDL3/SDL_opengl.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <set>
#include <tuple>

namespace sagomacad {
namespace {
ImU32 packed(ui::Color c){return ImGui::ColorConvertFloat4ToU32({c.r,c.g,c.b,c.a});}
std::vector<ui::ToolbarCommand> board_commands() {
    std::vector<ui::ToolbarCommand> result;std::set<std::string_view> seen;
    auto add=[&](auto commands){for(const auto& c:commands)if(seen.insert(c.id).second)result.push_back(c);};
    add(ui::file_menu_commands());add(ui::edit_toolbar_commands());
    for(const auto& tab:ui::toolbar_catalog())for(const auto& group:tab.groups)add(group.commands);
    return result;
}
}
unsigned Shell::design_icon_texture(std::string_view id,ui::IconAccent accent,float size,bool disabled) {
    if(!design_icons_) {
        design_icons_=std::make_unique<ui::Icons>(std::string(SAGOMACAD_ASSET_DIR)+"/icons/solidcraft/icons.json",
            [](const ui::IconBitmap& image){
                if(image.rgba.empty())return 0u;
                unsigned texture=0;glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);
                glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
                glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,image.pixels,image.pixels,0,GL_RGBA,GL_UNSIGNED_BYTE,image.rgba.data());glBindTexture(GL_TEXTURE_2D,0);return texture;
            },[](unsigned texture){glDeleteTextures(1,&texture);});
    }
    return design_icons_->texture(id,accent,state_.theme=="light",unsigned(std::lround(size*scale_)),disabled);
}
void Shell::draw_design_board(const ui::Theme& t) {
    const float z=float(scale_);const bool light=state_.theme=="light";
    const auto colors=ui::theme_colors(t);const auto commands=board_commands();const auto display=ImGui::GetIO().DisplaySize;
    const auto layout=layout_.design_board(display.x,display.y,z,t,colors.size(),commands.size());
    auto* dl=ImGui::GetBackgroundDrawList();
    auto text=[&](float x,float y,std::string_view label,ui::Color color,float size=0){dl->AddText(ImGui::GetFont(),size?size:ImGui::GetFontSize(),{x,y},packed(color),label.data(),label.data()+label.size());};
    auto block=[&](ui::LayoutRect r,std::string_view label){
        dl->AddRectFilled({r.x,r.y},{r.x+r.width,r.y+r.height},packed(t.panel),t.rounding*z);
        dl->AddRectFilled({r.x,r.y},{r.x+r.width,r.y+t.frame.heading_height*z},packed(t.visual.heading_bg),t.rounding*z);
        text(r.x+t.spacing*z,r.y+t.spacing*z*.5f,label,t.text);
    };
    auto draw_icon=[&](std::string_view id,ui::IconAccent accent,float x,float y,float size,bool disabled=false){
        auto texture=design_icon_texture(id,accent,size,disabled);
        if(texture)dl->AddImage(ImTextureID(texture),{x,y},{x+size*z,y+size*z});
    };
    text(layout.header.x,layout.header.y,"SagomaCad / Identità visiva",t.text,t.board.heading*z);
    const std::string subtitle=std::string(light?"CHIARO · PREDEFINITO":"SCURO")+"   /   Verde pistacchio   /   Interfaccia compatta   /   Proposta da approvare";
    text(layout.header.x,layout.header.y+t.board.row*z,subtitle,t.muted);
    block(layout.tokens,"TOKEN / Colori e stati del viewport");
    const auto columns=size_t(t.board.columns);const float column=layout.tokens.width/t.board.columns;
    for(size_t i=0;i<colors.size();++i) {
        const auto& [name,color]=colors[i];const float x=layout.tokens.x+float(i%columns)*column+t.spacing*z;
        const float y=layout.tokens.y+t.frame.heading_height*z+float(i/columns)*t.board.row*z+t.spacing*z;
        const float swatch=t.board.swatch*z;
        dl->AddRectFilled({x,y},{x+swatch,y+swatch},packed(color),t.rounding*z);
        dl->AddRect({x,y},{x+swatch,y+swatch},packed(t.border),t.rounding*z);
        text(x+swatch+t.spacing*z,y,name,t.text);
    }
    block(layout.icons,"CATALOGO / 16 · 24 · 32 px e stato disabilitato / SolidCraft per ruoli; fallback Lucide indicati");
    for(size_t i=0;i<commands.size();++i) {
        const auto& c=commands[i];const float x=layout.icons.x+float(i%columns)*column+t.spacing*z;
        const float y=layout.icons.y+t.frame.heading_height*z+float(i/columns)*t.board.icon_row*z+t.spacing*z*.5f;
        const std::string label=std::string(c.label)+" / "+(c.design_icon.empty()?"Lucide":std::string(c.design_icon));
        text(x,y,label,t.text);
        float at=x;
        for(float size:t.board.icon_sizes) {
            if(!c.design_icon.empty())draw_icon(c.design_icon,c.accent,at,y+t.model.row*z,size);
            else {auto texture=icon_texture(c.icon);if(texture)dl->AddImage(ImTextureID(texture),{at,y+t.model.row*z},{at+size*z,y+(t.model.row+size)*z},{0,0},{1,1},packed(t.text));}
            at+=(size+t.spacing*2)*z;
        }
        if(!c.design_icon.empty())draw_icon(c.design_icon,c.accent,at,y+t.model.row*z,t.board.disabled_icon,true);
    }
    block(layout.ribbon,"SOLIDO / Ribbon · 26 px");
    const auto create=ui::toolbar_catalog()[0].groups[0].commands;
    float x=layout.ribbon.x+t.padding*z,y=layout.ribbon.y+t.frame.heading_height*z+t.spacing*z;
    for(const auto& c:create) {
        draw_icon(c.design_icon,c.accent,x,y,t.board.ribbon_icon);
        text(x,y+(t.board.ribbon_icon+t.spacing)*z,c.label,t.text);x+=t.model.label_width*z;
    }
    block(layout.browser,"BROWSER / 16 px");
    y=layout.browser.y+(t.frame.heading_height+t.spacing)*z;
    for(auto [id,label,accent]:{std::tuple{"body","Corpo principale",ui::IconAccent::Solid},std::tuple{"sketch","Schizzo base",ui::IconAccent::Sketch},std::tuple{"origin","Origine",ui::IconAccent::Solid}}) {
        draw_icon(id,accent,layout.browser.x+t.padding*z,y,t.board.browser_icon);
        text(layout.browser.x+(t.padding+t.board.browser_icon+t.spacing)*z,y,label,t.text);y+=t.model.row*z;
    }
    block(layout.timeline,"TIMELINE / 20 px · marker · errore");
    x=layout.timeline.x+t.padding*z;y=layout.timeline.y+(t.frame.heading_height+t.padding)*z;
    for(auto id:{"sketch","extrude","fillet","hole","chamfer"}) {
        const bool error=std::string_view(id)=="chamfer";
        dl->AddRectFilled({x,y},{x+t.timeline.item*z,y+t.timeline.item*z},packed(error?t.visual.timeline_error:t.visual.timeline_item),t.rounding*z);
        draw_icon(id,std::string_view(id)=="sketch"?ui::IconAccent::Sketch:ui::IconAccent::Solid,x+(t.timeline.item-t.timeline.icon)*z*.5f,y+(t.timeline.item-t.timeline.icon)*z*.5f,t.timeline.icon);
        x+=(t.timeline.item+t.timeline.gap)*z;
    }
    dl->AddRectFilled({x,y-t.spacing*z},{x+t.timeline.marker_width*z,y+t.timeline.marker_height*z},packed(t.visual.timeline_marker));
    text(layout.timeline.x+t.padding*z,y+(t.timeline.item+t.spacing)*z,"Schizzo > Estrudi > Raccordo > Foro",t.muted);
    if(ui::frame::dialog("Estrudi / Esempio",nullptr,layout.dialog,t,z)) {
        ImGui::TextUnformatted("Profilo");ImGui::SameLine(t.model.label_width*z);ImGui::TextUnformatted("1 selezionato");
        ImGui::TextUnformatted("Distanza");ImGui::SameLine(t.model.label_width*z);ImGui::SetNextItemWidth(t.model.field_width*z);
        float value=20;ImGui::InputFloat("##sample_distance",&value,0,0,"%.1f mm",ImGuiInputTextFlags_ReadOnly);
        ImGui::TextUnformatted("Operazione");ImGui::SameLine(t.model.label_width*z);ImGui::TextUnformatted("Nuovo corpo");
        ui::frame::heading("ANTEPRIMA",t,z,true);ImGui::TextColored({t.muted.r,t.muted.g,t.muted.b,t.muted.a},"Applica solo alla conferma");
        ImGui::PushStyleColor(ImGuiCol_Button,ImVec4{t.accent.r,t.accent.g,t.accent.b,t.accent.a});
        ImGui::PushStyleColor(ImGuiCol_Text,ImVec4{t.visual.text_on_accent.r,t.visual.text_on_accent.g,t.visual.text_on_accent.b,t.visual.text_on_accent.a});
        ImGui::Button("OK",{t.model.field_width*z,0});ImGui::PopStyleColor(2);ImGui::SameLine();ImGui::Button("Annulla");
    }
    ImGui::End();
}
}
