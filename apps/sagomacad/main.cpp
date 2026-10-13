#include "shell.hpp"
#include "control.hpp"
#include "automation/dispatch.hpp"
#include "automation/session_file.hpp"
#include "engine/command.hpp"
#include <SDL3/SDL.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <random>
#include <string>

namespace {
std::string fresh_token() {
    static constexpr char hex[] = "0123456789abcdef";
    std::random_device random;
    std::string token;
    token.reserve(64);
    for (int i=0;i<32;++i) {
        const auto byte=static_cast<unsigned char>(random());
        token.push_back(hex[byte>>4]);
        token.push_back(hex[byte&15]);
    }
    return token;
}
}

int main(int argc,char** argv) {
    int port=0;
    bool control_requested=false;
    std::string token_file;
    std::filesystem::path session_file=sagomacad::automation::default_session_file();
    std::filesystem::path root=std::filesystem::current_path();
    for (int i=1;i<argc;++i) {
        const std::string arg=argv[i];
        if (arg=="--control" && i+1<argc) { control_requested=true; try { port=std::stoi(argv[++i]); } catch (...) { std::cerr<<"Invalid port\n"; return 2; } }
        else if (arg=="--control-token-file" && i+1<argc) token_file=argv[++i];
        else if (arg=="--session-file" && i+1<argc) session_file=argv[++i];
        else if (arg=="--automation-root" && i+1<argc) root=argv[++i];
        else { std::cerr<<"Unknown or incomplete option: "<<arg<<'\n'; return 2; }
    }
    if (control_requested && (port<0 || port>65535)) { std::cerr<<"Invalid port\n"; return 2; }
    sagomacad::Shell shell(1440,900,1,false,true);
    if (!shell.valid()) { std::cerr<<shell.error()<<'\n'; return 1; }
    sagomacad::engine::Engine engine;
    std::unique_ptr<sagomacad::ControlServer> server;
    {
        session_file=std::filesystem::absolute(session_file);
        std::string token;
        if (token_file.empty()) {
            const auto path=session_file.parent_path()/"control.token";
            std::filesystem::create_directories(path.parent_path());
            token=fresh_token();
            std::ofstream output(path,std::ios::binary|std::ios::trunc);
            if (!output) { std::cerr<<"Cannot write control token\n"; return 1; }
            output<<token<<'\n';
            output.close();
            std::error_code ignored;
            std::filesystem::permissions(path,std::filesystem::perms::owner_read|
                                         std::filesystem::perms::owner_write,
                                         std::filesystem::perm_options::replace,ignored);
            token_file=path.string();
        } else {
            token_file=std::filesystem::absolute(token_file).string();
            std::ifstream file(token_file);
            std::getline(file,token);
            if (token.empty()) { std::cerr<<"A nonempty --control-token-file is required\n"; return 2; }
        }
        root=std::filesystem::weakly_canonical(root);
        server=std::make_unique<sagomacad::ControlServer>(port,token,[&](const nlohmann::json& input)->nlohmann::json {
            const auto id=input.value("id",nlohmann::json());
            const auto method=input.value("method",std::string());
            if (method=="engine.commands" || method=="engine.execute") {
                auto reply=sagomacad::automation::dispatch(engine,method,input.value("params",nlohmann::json::object()));
                reply["id"]=id;
                return reply;
            }
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
        if (!sagomacad::automation::write_session_file(session_file,
                {server->port(),std::filesystem::path(token_file)},error)) {
            std::cerr<<error<<'\n'; return 1;
        }
    }
    while (shell.poll()) {
        if (server) server->pump();
        shell.render();
        SDL_Delay(16);
    }
    sagomacad::automation::SessionFile active;
    std::string ignored_error;
    if (sagomacad::automation::read_session_file(session_file,active,ignored_error) &&
        active.port==server->port()) {
        std::error_code ignored;
        std::filesystem::remove(session_file,ignored);
    }
    return 0;
}
