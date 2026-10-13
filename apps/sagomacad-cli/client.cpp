#include "client.hpp"
#include "automation/control_client.hpp"
#include "automation/dispatch.hpp"
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
using Json = nlohmann::json;
struct Options {
    std::optional<int> port;
    std::filesystem::path token_file;
    std::filesystem::path headless;
    std::filesystem::path session_file;
    std::string script;
    std::filesystem::path script_file;
    std::vector<std::string> positional;
};

bool parse_options(int argc, char** argv, Options& options, std::string& error) {
    for (int i=2; i<argc; ++i) {
        const std::string arg=argv[i];
        if (arg=="--port" || arg=="--token-file" || arg=="--headless" ||
            arg=="--session-file" || arg=="--script" || arg=="--script-file") {
            if (i+1>=argc) { error="Valore mancante per " + arg; return false; }
            const std::string value=argv[++i];
            if (arg=="--port") {
                try {
                    std::size_t used=0;
                    const int port=std::stoi(value,&used);
                    if (used!=value.size() || port<1 || port>65535) throw std::runtime_error("porta");
                    options.port=port;
                } catch (...) { error="Porta non valida"; return false; }
            } else if (arg=="--token-file") options.token_file=std::filesystem::u8path(value);
            else if (arg=="--headless") options.headless=std::filesystem::u8path(value);
            else if (arg=="--session-file") options.session_file=std::filesystem::u8path(value);
            else if (arg=="--script") options.script=value;
            else options.script_file=std::filesystem::u8path(value);
        } else if (arg.starts_with("--")) { error="Opzione sconosciuta: " + arg; return false; }
        else options.positional.push_back(arg);
    }
    if (!options.headless.empty() && (options.port || !options.token_file.empty())) {
        error="--headless non può essere combinato con --port o --token-file";
        return false;
    }
    return true;
}

bool load_headless(const std::filesystem::path& path, sagomacad::engine::Engine& engine, std::string& error) {
    if (path.extension()!=".sagomacad") { error="--headless richiede un file .sagomacad"; return false; }
    if (std::filesystem::exists(path) && !sagomacad::doc::load(path,engine.session().document,error)) return false;
    engine.session().path=std::filesystem::absolute(path);
    return true;
}

bool save_headless(const std::filesystem::path& path, const sagomacad::engine::Engine& engine,
                   const sagomacad::doc::Document& before, std::string& error) {
    if (path.empty() || before==engine.session().document) return true;
    return sagomacad::doc::save_atomic(engine.session().document,path,error);
}

bool remote_options(const Options& options, int& port, std::string& token, std::string& error) {
    sagomacad::automation::SessionFile descriptor;
    if (!options.port || options.token_file.empty()) {
        const auto session=options.session_file.empty() ? sagomacad::automation::default_session_file()
                                                        : options.session_file;
        if (!sagomacad::automation::read_session_file(session,descriptor,error)) return false;
    }
    port=options.port.value_or(descriptor.port);
    const auto token_path=options.token_file.empty() ? descriptor.token_file : options.token_file;
    std::ifstream input(token_path,std::ios::binary);
    if (!input || !std::getline(input,token) || token.empty()) {
        error="Impossibile leggere il token di controllo";
        return false;
    }
    return true;
}

bool remote_call(const Options& options, const std::string& method, const Json& params,
                 Json& response, std::string& error) {
    int port=0;
    std::string token;
    if (!remote_options(options,port,token,error)) return false;
    return sagomacad::automation::control_call(port,token,method,params,response,error);
}

bool script_steps(const Options& options, Json& steps, std::string& error) {
    if (!options.script.empty() && !options.script_file.empty()) {
        error="Usare --script oppure --script-file"; return false;
    }
    std::string source=options.script;
    if (!options.script_file.empty()) {
        std::ifstream input(options.script_file,std::ios::binary);
        if (!input) { error="Impossibile aprire lo script"; return false; }
        source.assign(std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>());
    }
    if (source.empty()) { error="Script mancante"; return false; }
    if (source.size()>1024*1024) { error="Script troppo grande"; return false; }
    steps=Json::parse(source,nullptr,false);
    if (!steps.is_array()) { error="Lo script deve essere un array JSON"; return false; }
    return true;
}

bool step_method(const Json& step, std::string& method, Json& params) {
    if (step.is_array() && step.size()==2 && step[0].is_string()) {
        method=step[0].get<std::string>(); params=step[1]; return true;
    }
    if (step.is_object() && step.contains("method") && step["method"].is_string()) {
        method=step["method"].get<std::string>(); params=step.value("params",Json::object()); return true;
    }
    return false;
}
}

int client_cli(int argc, char** argv) {
    try {
        const std::string mode=argv[1];
        Options options;
        std::string error;
        if (!parse_options(argc,argv,options,error)) { std::cerr<<error<<'\n'; return 2; }
        if (mode=="call") {
            if (options.positional.empty() || options.positional.size()>2 ||
                !options.script.empty() || !options.script_file.empty()) {
                std::cerr<<"Uso: sagomacad-cli call <metodo> [params JSON] [--port N --token-file PATH | --headless FILE]\n";
                return 2;
            }
            const std::string& method=options.positional[0];
            const Json params=options.positional.size()==2 ? Json::parse(options.positional[1],nullptr,false)
                                                           : Json::object();
            if (params.is_discarded()) { std::cerr<<"JSON dei parametri non valido\n"; return 2; }
            Json reply;
            if (!options.headless.empty()) {
                sagomacad::engine::Engine engine;
                if (!load_headless(options.headless,engine,error)) { std::cerr<<error<<'\n'; return 1; }
                const auto before=engine.session().document;
                reply=sagomacad::automation::dispatch(engine,method,params);
                reply["id"]=1;
                if (!reply.contains("error") && !save_headless(options.headless,engine,before,error)) {
                    std::cerr<<error<<'\n'; return 1;
                }
            } else if (!remote_call(options,method,params,reply,error)) {
                std::cerr<<error<<'\n'; return 1;
            }
            if (reply.contains("error")) { std::cerr<<reply.dump()<<'\n'; return 1; }
            std::cout<<reply.dump()<<'\n';
            return 0;
        }
        if (mode=="run") {
            if (!options.positional.empty()) { std::cerr<<"Argomenti posizionali inattesi\n"; return 2; }
            Json steps;
            if (!script_steps(options,steps,error)) { std::cerr<<error<<'\n'; return 2; }
            const bool remote=options.port || !options.token_file.empty() || !options.session_file.empty();
            if (remote && !options.headless.empty()) { std::cerr<<"--headless e connessione remota sono esclusivi\n"; return 2; }
            sagomacad::engine::Engine engine;
            if (!remote && !options.headless.empty() && !load_headless(options.headless,engine,error)) {
                std::cerr<<error<<'\n'; return 1;
            }
            const auto before=engine.session().document;
            Json results=Json::array();
            for (const auto& step : steps) {
                std::string method;
                Json params;
                if (!step_method(step,method,params)) { std::cerr<<"Passo dello script non valido\n"; return 2; }
                Json reply;
                if (remote) {
                    if (!remote_call(options,method,params,reply,error)) { std::cerr<<error<<'\n'; return 1; }
                } else {
                    reply=sagomacad::automation::dispatch(engine,method,params);
                    reply["id"]=results.size()+1;
                }
                if (reply.contains("error")) { std::cerr<<reply.dump()<<'\n'; return 1; }
                results.push_back(reply);
            }
            if (!remote && !save_headless(options.headless,engine,before,error)) {
                std::cerr<<error<<'\n'; return 1;
            }
            std::cout<<Json({{"results",results}}).dump()<<'\n';
            return 0;
        }
        std::cerr<<"Comando CLI sconosciuto\n";
        return 2;
    } catch (const std::exception& e) {
        std::cerr<<e.what()<<'\n';
        return 1;
    }
}
