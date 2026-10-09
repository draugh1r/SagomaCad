#include "shell.hpp"
#include "control.hpp"
#include <SDL3/SDL.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

int main(int argc,char** argv) {
    int port=0;
    bool control_requested=false;
    std::string token_file;
    std::filesystem::path root=std::filesystem::current_path();
    for (int i=1;i<argc;++i) {
        const std::string arg=argv[i];
        if (arg=="--control" && i+1<argc) { control_requested=true; try { port=std::stoi(argv[++i]); } catch (...) { std::cerr<<"Invalid port\n"; return 2; } }
        else if (arg=="--control-token-file" && i+1<argc) token_file=argv[++i];
        else if (arg=="--automation-root" && i+1<argc) root=argv[++i];
        else { std::cerr<<"Unknown or incomplete option: "<<arg<<'\n'; return 2; }
    }
    if (control_requested && (port<1 || port>65535)) { std::cerr<<"Invalid port\n"; return 2; }
    sagomacad::Shell shell(1440,900,1,false,false);
    if (!shell.valid()) { std::cerr<<shell.error()<<'\n'; return 1; }
    std::unique_ptr<sagomacad::ControlServer> server;
    if (port) {
        std::ifstream file(token_file);
        std::string token; std::getline(file,token);
        if (token.empty()) { std::cerr<<"A nonempty --control-token-file is required\n"; return 2; }
        root=std::filesystem::weakly_canonical(root);
        server=std::make_unique<sagomacad::ControlServer>(port,token,[&](const nlohmann::json& input)->nlohmann::json {
            const auto id=input.value("id",nlohmann::json());
            const auto method=input.value("method",std::string());
            if (method=="ui.inspect") return {{"id",id},{"result",shell.inspect()}};
            if (method=="ui.screenshot") {
                const auto params=input.value("params",nlohmann::json::object());
                if (!params.is_object()) return {{"id",id},{"error",{{"message","params must be an object"}}}};
                const auto name=params.value("path",std::string("screenshot.png"));
                const std::filesystem::path relative(name);
                if (relative.empty() || relative.is_absolute() || name.find("..")!=std::string::npos)
                    return {{"id",id},{"error",{{"message","path must be relative and inside automation root"}}}};
                const auto path=std::filesystem::weakly_canonical(root/relative);
                auto root_part=root.begin();
                auto path_part=path.begin();
                while (root_part!=root.end() && path_part!=path.end() && *root_part==*path_part) {
                    ++root_part; ++path_part;
                }
                if (root_part!=root.end())
                    return {{"id",id},{"error",{{"message","path escapes automation root"}}}};
                shell.render();
                std::string error;
                if (!shell.screenshot(path.string(),error)) return {{"id",id},{"error",{{"message",error}}}};
                return {{"id",id},{"result",{{"path",path.string()}}}};
            }
            return {{"id",id},{"error",{{"message","unknown method"}}}};
        });
        std::string error;
        if (!server->start(error)) { std::cerr<<error<<'\n'; return 1; }
    }
    while (shell.poll()) {
        if (server) server->pump();
        shell.render();
        SDL_Delay(16);
    }
    return 0;
}
