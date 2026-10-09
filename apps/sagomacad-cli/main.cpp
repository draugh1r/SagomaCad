#include "shell.hpp"
#include <iostream>
#include <string>

int main(int argc,char** argv) {
    if (argc<2 || std::string(argv[1])!="snapshot") {
        std::cerr<<"Usage: sagomacad-cli snapshot --size WIDTHxHEIGHT [--scale 1|2] --out file.png [--theme dark|light] [--glyph-test] [--script JSON]\n";
        return 2;
    }
    int width=1440,height=900,scale=1;
    std::string output,theme="dark",script="[]";
    bool glyph_test=false;
    for (int i=2;i<argc;++i) {
        const std::string arg=argv[i];
        if (arg=="--glyph-test") { glyph_test=true; continue; }
        if (i+1>=argc) { std::cerr<<"Missing value for "<<arg<<'\n'; return 2; }
        const std::string value=argv[++i];
        try {
            if (arg=="--size") {
                auto x=value.find('x'); if (x==std::string::npos) throw std::invalid_argument("size");
                width=std::stoi(value.substr(0,x)); height=std::stoi(value.substr(x+1));
            } else if (arg=="--scale") scale=std::stoi(value);
            else if (arg=="--out") output=value;
            else if (arg=="--theme") theme=value;
            else if (arg=="--script") script=value;
            else throw std::invalid_argument("unknown option");
        } catch (...) { std::cerr<<"Invalid "<<arg<<'\n'; return 2; }
    }
    if (width<320 || height<240 || width>4096 || height>4096 || scale<1 || scale>2 ||
        width*scale>8192 || height*scale>8192 || output.empty() || (theme!="dark" && theme!="light")) {
        std::cerr<<"Invalid snapshot options\n"; return 2;
    }
    auto steps=nlohmann::json::parse(script,nullptr,false);
    if (!steps.is_array()) { std::cerr<<"--script must be a JSON array\n"; return 2; }
    for (const auto& step:steps) {
        if (!step.is_array() || step.size()!=2 || step[0]!="ui.set" || !step[1].is_object()) {
            std::cerr<<"M0 snapshot script supports only [\"ui.set\",{\"theme\":...}]\n"; return 2;
        }
        if (step[1].contains("theme")) {
            if (!step[1]["theme"].is_string()) return 2;
            theme=step[1]["theme"].get<std::string>();
            if (theme!="dark" && theme!="light") return 2;
        }
    }
    sagomacad::Shell shell(width,height,scale,true,theme=="light");
    if (!shell.valid()) { std::cerr<<shell.error()<<'\n'; return 1; }
    if (glyph_test && !shell.glyphs_available()) { std::cerr<<"Missing Latin glyphs in Inter font\n"; return 1; }
    shell.set_glyph_sample(glyph_test);
    for (int frame=0;frame<3;++frame) { shell.poll(); shell.render(); }
    std::string error;
    if (!shell.screenshot(output,error)) { std::cerr<<error<<'\n'; return 1; }
    std::cout<<output<<'\n';
    return 0;
}
