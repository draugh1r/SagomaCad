#include "ui/theme.hpp"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <set>
#include <stdexcept>
using namespace sagomacad::ui;
void check(bool b,const std::string& m){if(!b)throw std::runtime_error(m);}
double luminance(Color c){auto linear=[](double v){return v<=.04045?v/12.92:std::pow((v+.055)/1.055,2.4);};return .2126*linear(c.r)+.7152*linear(c.g)+.0722*linear(c.b);}
double contrast(Color a,Color b){auto x=luminance(a),y=luminance(b);return (std::max(x,y)+.05)/(std::min(x,y)+.05);}
int main(int argc,char** argv)try {
    check(argc==2,"project root");
    for(bool is_light:{true,false}) {
        const auto& t=theme(is_light);std::set<std::string_view> names;
        for(const auto& [name,c]:theme_colors(t)) {
            check(names.insert(name).second,"duplicate token");
            for(float v:{c.r,c.g,c.b,c.a})check(std::isfinite(v)&&v>=0&&v<=1,"invalid color: "+std::string(name));
        }
        check(names.size()>70,"complete viewport states");
        for(auto background:{t.panel,t.panel_alt,t.visual.dialog_bg,t.visual.heading_bg,t.visual.accent_soft})
            check(contrast(t.text,background)>=4.5,"text contrast");
        check(contrast(t.muted,t.panel)>=4.5,"secondary text contrast");
        check(contrast(t.table_header_text,t.table_header)>=4.5,"table heading contrast");
        check(contrast(t.visual.text_on_accent,t.accent)>=4.5,"primary button contrast");
        if(is_light) {
            for(auto surface:{t.background,t.panel,t.panel_alt,t.viewport,t.table_header,
                t.visual.app_bar,t.visual.toolbar,t.visual.panel_header,t.visual.dialog_bg,
                t.visual.heading_bg,t.visual.subheading_bg,t.visual.field,
                t.visual.viewport_top,t.visual.viewport_bottom,t.visual.overlay,t.visual.glyph_bg})
                check(surface.r==1&&surface.g==1&&surface.b==1,"ordinary light surfaces must be white");
            for(auto neutral:{t.text,t.muted,t.border})
                check(neutral.r==neutral.g&&neutral.g==neutral.b,"neutral UI must have no green tint");
        }
        std::cout<<(is_light?"light":"dark")<<": "<<names.size()<<" tokens, text contrast "<<contrast(t.text,t.panel)<<", secondary "<<contrast(t.muted,t.panel)<<'\n';
    }
    const std::regex literal(R"((?:\bColor(?:\s+\w+)?|\bImVec4)\s*\{\s*[.0-9]|\bIM_COL32\s*\(\s*[.0-9]|(?:ColorConvertFloat4ToU32|PushStyleColor)\s*\([^\n]*\{\s*[.0-9]|\bglClearColor\s*\(\s*[.0-9])");
    for(auto folder:{"src/ui","apps/common"})for(const auto& entry:std::filesystem::recursive_directory_iterator(std::filesystem::path(argv[1])/folder)) {
        const auto p=entry.path();if(p.extension()!=".cpp"&&p.extension()!=".hpp")continue;
        if(p.filename()=="theme.hpp")continue;
        std::ifstream input(p);std::string source((std::istreambuf_iterator<char>(input)),{});
        check(!std::regex_search(source,literal),"literal color outside theme: "+p.string());
    }
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
