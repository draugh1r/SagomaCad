#include "mcp.hpp"
#include "mcp_visual.hpp"
#include "automation/mcp/server.hpp"
#include "automation/dispatch.hpp"
#include "automation/control_client.hpp"
#include "automation/session_file.hpp"
#include "doc/document.hpp"
#include "engine/command.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace {
namespace fs=std::filesystem;
using Json=nlohmann::json;
struct Remote { int port=0; std::string token; };

bool connect_session(const fs::path& path,Remote& remote) {
    sagomacad::automation::SessionFile descriptor;
    std::string error;
    if (!sagomacad::automation::read_session_file(path,descriptor,error)) return false;
    std::ifstream input(descriptor.token_file,std::ios::binary);
    if (!input || !std::getline(input,remote.token) || remote.token.empty()) return false;
    remote.port=descriptor.port;
    Json response;
    return sagomacad::automation::control_call(remote.port,remote.token,"engine.commands",Json::object(),response,error)
        && response.contains("result");
}
bool read_explicit(int port,const fs::path& token_path,Remote& remote) {
    if (port<1 || port>65535 || token_path.empty()) return false;
    std::ifstream input(token_path,std::ios::binary);
    if (!input || !std::getline(input,remote.token) || remote.token.empty()) return false;
    remote.port=port;
    Json response;
    std::string error;
    return sagomacad::automation::control_call(port,remote.token,"engine.commands",Json::object(),response,error)
        && response.contains("result");
}
}

int mcp_cli(int argc,char** argv) {
    bool force_headless=false,explicit_bridge=false;
    int port=0;
    fs::path headless_file,token_file,session_file;
    std::vector<fs::path> roots;
    for (int i=2;i<argc;++i) {
        const std::string option=argv[i];
        if (option=="--headless") {
            force_headless=true;
            if (i+1<argc && std::string_view(argv[i+1]).substr(0,2)!="--") headless_file=fs::u8path(argv[++i]);
        } else if (option=="--bridge" && i+1<argc) {
            explicit_bridge=true;
            try { port=std::stoi(argv[++i]); } catch (...) { std::cerr<<"Porta bridge non valida\n"; return 2; }
        } else if (option=="--token-file" && i+1<argc) token_file=fs::u8path(argv[++i]);
        else if (option=="--session-file" && i+1<argc) session_file=fs::u8path(argv[++i]);
        else if (option=="--allow-root" && i+1<argc) roots.push_back(fs::weakly_canonical(fs::u8path(argv[++i])));
        else { std::cerr<<"Opzione MCP non valida: "<<option<<'\n'; return 2; }
    }
    if (force_headless && (explicit_bridge || !token_file.empty() || !session_file.empty())) {
        std::cerr<<"--headless e bridge sono esclusivi\n"; return 2;
    }
    if (roots.empty()) roots.push_back(fs::weakly_canonical(fs::path(SAGOMACAD_PROJECT_ROOT)));
    sagomacad::engine::Engine engine;
    if (!headless_file.empty()) {
        if (headless_file.extension()!=".sagomacad") { std::cerr<<"--headless richiede un file .sagomacad\n"; return 2; }
        std::string error;
        if (fs::exists(headless_file) && !sagomacad::doc::load(headless_file,engine.session().document,error)) {
            std::cerr<<error<<'\n'; return 1;
        }
        engine.session().path=fs::absolute(headless_file);
    }
    Remote remote;
    bool bridge=false;
    if (!force_headless) {
        if (explicit_bridge) bridge=read_explicit(port,token_file,remote);
        else if (!session_file.empty()) bridge=connect_session(session_file,remote);
        else {
            const std::vector<fs::path> candidates={
                sagomacad::automation::default_session_file(),
                fs::absolute(fs::path(argv[0])).parent_path()/".sagomacad"/"session.json",
                fs::path(SAGOMACAD_PROJECT_ROOT)/".sagomacad"/"session.json",
                fs::absolute(fs::path(argv[0])).parent_path().parent_path()/"sagomacad"/".sagomacad"/"session.json"
            };
            for (const auto& candidate:candidates) if (connect_session(candidate,remote)) { bridge=true; break; }
        }
        if ((explicit_bridge || !session_file.empty()) && !bridge) {
            std::cerr<<"Bridge SagomaCad non raggiungibile o token non valido\n"; return 1;
        }
    }
    const auto commands=[&](std::string_view method,const Json& params)->Json {
        if (!bridge) return sagomacad::automation::dispatch(engine,method,params);
        Json reply;
        std::string error;
        if (!sagomacad::automation::control_call(remote.port,remote.token,std::string(method),params,reply,error))
            return {{"error",{{"message",error}}}};
        return reply;
    };
    const auto visuals=[&](std::string_view name,const Json& args)->Json {
        return mcp_visual_tool(name,args,commands,bridge,engine);
    };
    sagomacad::automation::mcp::Server server(commands,visuals,bridge,std::move(roots));
    server.serve(std::cin,std::cout);
    return 0;
}
