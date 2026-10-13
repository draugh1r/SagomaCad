#include "shell.hpp"
#include "fuzz.hpp"
#include "client.hpp"
#include "mcp.hpp"
#include "ui/parameter_editor.hpp"
#include <cstdio>
#include <iostream>
#include <string>
#include <array>

int main(int argc,char** argv) {
    if (argc==2 && std::string(argv[1])=="fuzz-commands") return fuzz_commands();
    if (argc>=2 && (std::string(argv[1])=="call" || std::string(argv[1])=="run"))
        return client_cli(argc,argv);
    if (argc>=2 && std::string(argv[1])=="mcp") return mcp_cli(argc,argv);
    if (argc<2 || std::string(argv[1])!="snapshot") {
        std::cerr<<"Usage: sagomacad-cli snapshot --size WIDTHxHEIGHT [--scale 1|2] --out file.png [--theme dark|light] [--glyph-test] [--script JSON]\n";
        return 2;
    }
    int width=1440,height=900,scale=1;
    std::string output,theme="light",script="[]";
    bool glyph_test=false,sketch_mode=false,palette_open=false,parameters_open=false,export_dialog=false;
    nlohmann::json parameter_draft,feature_draft;
    std::string feature_edit_id;
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
    sagomacad::engine::Engine engine;
    for (const auto& step:steps) {
        if (!step.is_array() || step.size()!=2 || !step[0].is_string() || !step[1].is_object()) return 2;
        const auto method=step[0].get<std::string>();
        const auto& params=step[1];
        if (method=="engine.execute") {
            if (!params.contains("command") || !params["command"].is_string()) return 2;
            const auto result=engine.execute(params["command"].get<std::string>(),params.value("params",nlohmann::json::object()));
            if (!result.ok) { std::cerr<<result.error<<'\n'; return 1; }
        } else if (method=="ui.set") {
            if (params.contains("theme")) {
                if (!params["theme"].is_string()) return 2;
                theme=params["theme"].get<std::string>();
                if (theme!="dark" && theme!="light") return 2;
            }
            for (auto [key,target]:{std::pair{"sketch_mode",&sketch_mode},std::pair{"palette",&palette_open},std::pair{"parameters",&parameters_open},std::pair{"export_dialog",&export_dialog}}) {
                if (params.contains(key)) { if (!params[key].is_boolean()) return 2; *target=params[key].get<bool>(); }
            }
            if (params.contains("parameter_draft")) {
                if (!params["parameter_draft"].is_object()) return 2;
                parameter_draft=params["parameter_draft"];
                parameters_open=true;
            }
            if (params.contains("feature_edit")) {
                if (!params["feature_edit"].is_string()) return 2;
                feature_edit_id=params["feature_edit"].get<std::string>();
            }
            if (params.contains("feature_draft")) {
                if (!params["feature_draft"].is_object()) return 2;
                feature_draft=params["feature_draft"];
            }
        } else { std::cerr<<"Unknown snapshot method: "<<method<<'\n'; return 2; }
    }
    sagomacad::Shell shell(width,height,scale,true,theme=="light");
    if (!shell.valid()) { std::cerr<<shell.error()<<'\n'; return 1; }
    shell.bind_engine(&engine);
    shell.state().sketch_mode=sketch_mode;
    shell.state().palette_open=palette_open;
    shell.state().parameters_open=parameters_open;
    if (export_dialog) {
        shell.state().file_dialog_open=true;
        shell.state().file_command="export.stl";
    }
    if (!feature_edit_id.empty()) {
        if (!shell.open_feature_editor(feature_edit_id)) return 2;
        const auto type=engine.session().document.features.at(feature_edit_id).type;
        const std::array<const char*,3> keys=type=="primitive.box"?std::array<const char*,3>{"width","depth","height"}:
            type=="primitive.cylinder"?std::array<const char*,3>{"radius","height",nullptr}:std::array<const char*,3>{"radius",nullptr,nullptr};
        for (int i=0;i<3;++i) if (keys[i] && feature_draft.contains(keys[i])) {
            if (!feature_draft[keys[i]].is_string()) return 2;
            const auto value=feature_draft[keys[i]].get<std::string>();
            if (value.size()>=shell.state().primitive_dimensions[i].size()) return 2;
            std::snprintf(shell.state().primitive_dimensions[i].data(),shell.state().primitive_dimensions[i].size(),"%s",value.c_str());
        }
    }
    if (parameter_draft.is_object()) {
        auto& state=shell.state();
        state.parameter_editing=true;
        auto copy=[](auto& buffer,const nlohmann::json& source,const char* key)->bool {
            if (!source.contains(key)) return true;
            if (!source[key].is_string()) return false;
            const auto value=source[key].get<std::string>();
            if (value.size()>=buffer.size()) return false;
            std::snprintf(buffer.data(),buffer.size(),"%s",value.c_str());
            return true;
        };
        if (!copy(state.parameter_name,parameter_draft,"name") ||
            !copy(state.parameter_unit,parameter_draft,"unit") ||
            !copy(state.parameter_expression,parameter_draft,"expression") ||
            !copy(state.parameter_comment,parameter_draft,"comment")) return 2;
        if (parameter_draft.contains("submit")) {
            if (!parameter_draft["submit"].is_boolean()) return 2;
            if (parameter_draft["submit"].get<bool>()) sagomacad::ui::submit_parameter(state,engine);
        }
    }
    if (glyph_test && !shell.glyphs_available()) { std::cerr<<"Missing Latin glyphs in Inter font\n"; return 1; }
    shell.set_glyph_sample(glyph_test);
    for (int frame=0;frame<3;++frame) { shell.poll(); shell.render(); }
    std::string error;
    if (!shell.screenshot(output,error)) { std::cerr<<error<<'\n'; return 1; }
    std::cout<<output<<'\n';
    return 0;
}
