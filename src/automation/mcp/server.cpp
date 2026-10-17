#include "automation/mcp/server.hpp"
#include "modeling_guide.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <string>

namespace sagomacad::automation::mcp {
namespace {
Json rpc_error(const Json& id,int number,std::string code,std::string message,std::string suggestion={}) {
    Json error={{"code",number},{"message",std::move(message)},
                {"data",{{"code",std::move(code)},{"suggestion",std::move(suggestion)}}}};
    return {{"jsonrpc","2.0"},{"id",id},{"error",std::move(error)}};
}
Json tool_error(std::string code,std::string message,std::string suggestion={}) {
    Json data={{"code",std::move(code)},{"message",std::move(message)},{"suggestion",std::move(suggestion)}};
    return {{"isError",true},{"content",Json::array({{{"type","text"},{"text",data.dump()}}})},
            {"structuredContent",data}};
}
Json tool_success(const Json& value) {
    return {{"content",Json::array({{{"type","text"},{"text",value.dump()}}})},
            {"structuredContent",value.is_object()?value:Json{{"items",value}}}};
}
Json schema(std::initializer_list<std::pair<std::string,Json>> properties={},
            std::initializer_list<std::string> required={}) {
    Json p=Json::object();
    for (const auto& [name,value]:properties) p[name]=value;
    return {{"type","object"},{"properties",p},{"required",required},{"additionalProperties",false}};
}
Json text_schema() { return {{"type","string"}}; }
Json bool_schema() { return {{"type","boolean"}}; }
Json object_schema() { return {{"type","object"}}; }
Json tools_catalog() {
    const auto entry=[](const char* name,const char* description,Json input) {
        return Json{{"name",name},{"description",description},{"inputSchema",std::move(input)}};
    };
    return Json::array({
        entry("commands_list","Elenca i comandi disponibili e il loro stato",schema({{"filter",text_schema()}})),
        entry("command_run","Esegue un comando del registro SagomaCad",schema({{"command",text_schema()},{"params",object_schema()}},{"command"})),
        entry("batch_run","Esegue da 1 a 64 comandi come un solo passo di undo",schema({{"steps",{{"type","array"},{"minItems",1},{"maxItems",64}}},{"stop_on_error",bool_schema()}},{"steps"})),
        entry("undo","Annulla l'ultima modifica",schema()),
        entry("redo","Ripete l'ultima modifica annullata",schema()),
        entry("document_inspect","Legge documento, parametri, feature e storia",schema()),
        entry("sketch_inspect","Entità, vincoli, quote, profili e stato del solver",schema({{"sketch",text_schema()}})),
        entry("feature_get","Dati, stato e messaggio di errore di una feature",schema({{"feature_id",text_schema()}},{"feature_id"})),
        entry("document_open","Apre un documento in una radice consentita",schema({{"path",text_schema()}},{"path"})),
        entry("document_save","Salva un documento in una radice consentita",schema({{"path",text_schema()},{"overwrite",bool_schema()}})),
        entry("render_view","Renderizza un PNG da vista iso, top, front o right",schema({{"view",{{"type","string"},{"enum",Json::array({"iso","top","front","right"})}}},{"size",text_schema()}},{"view"})),
        entry("geometry_query","Facce, spigoli e vertici con nomi stabili, firme e filtri geometrici",schema({{"body",text_schema()},{"kind",text_schema()},{"type",text_schema()},{"parallel_to",{{"type","array"},{"minItems",3},{"maxItems",3},{"items",{{"type","number"}}}}},{"normal",{{"type","array"},{"minItems",3},{"maxItems",3},{"items",{{"type","number"}}}}},{"limit",{{"type","integer"}}}})),
        entry("measure","Distanza minima e angolo fra due entità stabili",schema({{"a",object_schema()},{"b",object_schema()}},{"a","b"})),
        entry("selection_get","Legge la selezione utente nell'app aperta (solo bridge)",schema()),
        entry("mass_properties","Volume, area, baricentro e bounding box di un corpo",schema({{"body",text_schema()}})),
        entry("export","Esporta STL nella cartella export consentita",schema({{"format",{{"type","string"},{"enum",Json::array({"stl"})}}},{"path",text_schema()},{"body",text_schema()},{"stage",text_schema()},{"tolerance",{{"type","number"}}},{"overwrite",bool_schema()}},{"format","path"})),
        entry("ui_screenshot","Cattura la finestra dell'app aperta (solo bridge)",schema())
    });
}
bool valid_keys(const Json& value,std::initializer_list<std::string_view> keys) {
    if (!value.is_object()) return false;
    for (auto it=value.begin();it!=value.end();++it)
        if (std::find(keys.begin(),keys.end(),std::string_view(it.key()))==keys.end()) return false;
    return true;
}
bool string_field(const Json& value,const char* key,bool required=false) {
    return (!required && !value.contains(key)) || (value.contains(key) && value[key].is_string());
}
}

Server::Server(Handler commands,Handler visuals,bool bridge,std::vector<std::filesystem::path> roots)
    :commands_(std::move(commands)),visuals_(std::move(visuals)),bridge_(bridge),roots_(std::move(roots)) {}

bool Server::in_roots(const std::filesystem::path& path) const {
    const auto canonical=std::filesystem::weakly_canonical(path);
    for (const auto& root:roots_) {
        auto a=canonical.begin(),b=root.begin();
        while (a!=canonical.end() && b!=root.end() && *a==*b) { ++a; ++b; }
        if (b==root.end()) return true;
    }
    return false;
}

std::optional<std::filesystem::path> Server::document_path(const Json& a,bool saving,std::string& error) {
    std::filesystem::path relative;
    if (a.contains("path")) {
        const auto value=a["path"].get<std::string>();
        relative=std::filesystem::u8path(value);
        if (relative.empty() || relative.is_absolute() || relative.has_root_name()) {
            error="Usa un percorso relativo a una radice consentita"; return std::nullopt;
        }
        for (const auto& part:relative) if (part=="..") { error="Il percorso non può contenere .."; return std::nullopt; }
        relative=roots_.front()/relative;
    } else if (saving) {
        const auto reply=commands_("engine.execute",{{"command","document.inspect"},{"params",Json::object()}});
        if (reply.contains("error") || !reply.at("result").contains("path") || !reply["result"]["path"].is_string()) {
            error="Impossibile conoscere il percorso del documento"; return std::nullopt;
        }
        relative=std::filesystem::u8path(reply["result"]["path"].get<std::string>());
    } else { error="path è obbligatorio"; return std::nullopt; }
    if (relative.empty() || relative.extension()!=".sagomacad" || !in_roots(relative)) {
        error="Il file .sagomacad deve essere in una radice consentita"; return std::nullopt;
    }
    return std::filesystem::weakly_canonical(relative);
}

Json Server::invoke(std::string_view method,const Json& params) {
    const auto reply=commands_(method,params);
    if (!reply.is_object()) return tool_error("bridge.invalid_reply","Risposta del motore non valida","Riprova o riavvia l'app");
    if (reply.contains("error")) {
        const auto message=reply["error"].is_object()?reply["error"].value("message",std::string("Errore del motore")):"Errore del motore";
        return tool_error("command.failed",message,"Controlla i parametri con commands_list");
    }
    if (!reply.contains("result")) return tool_error("bridge.invalid_reply","Risposta del motore senza result");
    return tool_success(reply["result"]);
}

Json Server::tool(std::string_view name,const Json& a) {
    if (name=="commands_list") {
        if (!valid_keys(a,{"filter"}) || !string_field(a,"filter")) return tool_error("mcp.invalid_arguments","filter deve essere una stringa");
        auto result=invoke("engine.commands",Json::object());
        if (result.value("isError",false)) return result;
        auto commands=result["structuredContent"]["items"];
        if (a.contains("filter")) {
            auto filtered=Json::array();
            const auto needle=a["filter"].get<std::string>();
            for (const auto& command:commands)
                if (command.value("id",std::string()).find(needle)!=std::string::npos ||
                    command.value("label",std::string()).find(needle)!=std::string::npos) filtered.push_back(command);
            commands=std::move(filtered);
        }
        return tool_success({{"commands",std::move(commands)}});
    }
    if (name=="command_run") {
        if (!valid_keys(a,{"command","params"}) || !string_field(a,"command",true) ||
            (a.contains("params") && !a["params"].is_object()))
            return tool_error("mcp.invalid_arguments","command deve essere una stringa e params un oggetto");
        const auto command=a["command"].get<std::string>();
        if (command=="document.open" || command=="document.save" || command=="document.saveAs" || command=="export.stl")
            return tool_error("mcp.path_policy","Usa document_open, document_save o export per i file",
                              "Questi tool verificano radice consentita e overwrite");
        return invoke("engine.execute",{{"command",a["command"]},{"params",a.value("params",Json::object())}});
    }
    if (name=="batch_run") {
        if (!valid_keys(a,{"steps","stop_on_error"}) || !a.contains("steps") || !a["steps"].is_array() ||
            (a.contains("stop_on_error") && !a["stop_on_error"].is_boolean()))
            return tool_error("mcp.invalid_arguments","steps deve essere un array e stop_on_error un booleano");
        return invoke("engine.batch",{{"steps",a["steps"]},{"stop_on_error",a.value("stop_on_error",true)}});
    }
    if (name=="undo" || name=="redo") {
        if (!a.empty()) return tool_error("mcp.invalid_arguments","Questo tool non accetta argomenti");
        return invoke("engine.execute",{{"command",name=="undo"?"edit.undo":"edit.redo"},{"params",Json::object()}});
    }
    if (name=="document_inspect") {
        if (!a.empty()) return tool_error("mcp.invalid_arguments","Questo tool non accetta argomenti");
        return invoke("engine.execute",{{"command","document.inspect"},{"params",Json::object()}});
    }
    if (name=="document_open" || name=="document_save") {
        const bool saving=name=="document_save";
        if (!valid_keys(a,saving?std::initializer_list<std::string_view>{"path","overwrite"}:
                              std::initializer_list<std::string_view>{"path"}) ||
            !string_field(a,"path",!saving) ||
            (a.contains("overwrite") && !a["overwrite"].is_boolean()))
            return tool_error("mcp.invalid_arguments","path deve essere una stringa e overwrite un booleano");
        std::string error;
        const auto path=document_path(a,saving,error);
        if (!path) return tool_error("mcp.path_denied",error,"Usa un percorso relativo alla radice consentita");
        if (saving && std::filesystem::exists(*path) && !a.value("overwrite",false))
            return tool_error("mcp.overwrite_required","Il file esiste già: passa overwrite=true");
        const auto bytes=path->u8string();
        const std::string utf8(bytes.begin(),bytes.end());
        return invoke("engine.execute",{{"command",saving?"document.saveAs":"document.open"},{"params",{{"path",utf8}}}});
    }
    if (name=="sketch_inspect") {
        if (!valid_keys(a,{"sketch"}) || !string_field(a,"sketch"))
            return tool_error("mcp.invalid_arguments","sketch deve essere una stringa");
        return invoke("engine.execute",{{"command","sketch.inspect"},{"params",a}});
    }
    if(name=="feature_get"){
        if(!valid_keys(a,{"feature_id"})||!string_field(a,"feature_id",true))return tool_error("mcp.invalid_arguments","feature_id deve essere una stringa");
        return invoke("engine.execute",{{"command","feature.get"},{"params",a}});
    }
    if (name=="mass_properties") {
        if (!valid_keys(a,{"body"}) || !string_field(a,"body"))
            return tool_error("mcp.invalid_arguments","body deve essere una stringa");
        return invoke("engine.execute",{{"command","model.mass_properties"},{"params",a}});
    }
    if (name=="export") {
        if (!valid_keys(a,{"format","path","body","stage","tolerance","overwrite"}) ||
            !string_field(a,"format",true) || !string_field(a,"path",true) ||
            !string_field(a,"body") || !string_field(a,"stage") ||
            (a.contains("tolerance") && !a["tolerance"].is_number()) ||
            (a.contains("overwrite") && !a["overwrite"].is_boolean()))
            return tool_error("mcp.invalid_arguments","Parametri export non validi");
        if (a["format"]!="stl") return tool_error("mcp.unsupported_format","Solo STL è disponibile in M2");
        auto filename=std::filesystem::u8path(a["path"].get<std::string>());
        if (filename.empty() || filename.is_absolute() || filename.has_root_name() ||
            filename.has_parent_path() || filename=="." || filename=="..")
            return tool_error("mcp.path_denied","export.path deve contenere solo il nome del file");
        auto extension=filename.extension().string();
        std::transform(extension.begin(),extension.end(),extension.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
        if (extension!=".stl") filename.replace_extension(".stl");
        const auto destination=std::filesystem::path(SAGOMACAD_EXPORT_DIR)/filename;
        if (!in_roots(destination)) return tool_error("mcp.path_denied","La cartella export non è in una radice consentita");
        if (std::filesystem::exists(destination) && !a.value("overwrite",false))
            return tool_error("mcp.overwrite_required","Lo STL esiste già: passa overwrite=true");
        Json params={{"path",filename.string()}};
        for (const auto* key:{"body","stage","tolerance"}) if (a.contains(key)) params[key]=a[key];
        return invoke("engine.execute",{{"command","export.stl"},{"params",params}});
    }
    if(name=="geometry_query")return invoke("engine.execute",{{"command","geometry.query"},{"params",a}});
    if(name=="measure")return invoke("engine.execute",{{"command","inspect.measure"},{"params",a}});
    if(name=="selection_get"){
        if(!a.is_object()||!a.empty())return tool_error("mcp.invalid_arguments","selection_get non accetta argomenti");
        if(!bridge_)return tool_error("mcp.bridge_required","selection_get richiede l'app aperta");
        return invoke("engine.execute",{{"command","selection.get"},{"params",a}});
    }
    if (name=="render_view") return visuals_(name,a);
    if (name=="ui_screenshot") {
        if (!bridge_) return tool_error("mcp.bridge_required","ui_screenshot richiede l'app aperta","Avvia SagomaCad e usa la modalità bridge");
        return visuals_(name,a);
    }
    return tool_error("mcp.unknown_tool","Tool MCP sconosciuto: "+std::string(name),"Consulta tools/list");
}

std::optional<Json> Server::handle(const Json& request) {
    if (request.is_array()) {
        if (request.empty()) return rpc_error(nullptr,-32600,"mcp.invalid_request","Il batch JSON-RPC è vuoto");
        Json replies=Json::array();
        for (const auto& item:request) if (const auto reply=handle(item)) replies.push_back(*reply);
        if (replies.empty()) return std::nullopt;
        return replies;
    }
    if (!request.is_object()) return rpc_error(nullptr,-32600,"mcp.invalid_request","La richiesta deve essere un oggetto JSON");
    const auto id=request.value("id",Json(nullptr));
    if (!request.contains("jsonrpc") || request["jsonrpc"]!="2.0" ||
        !request.contains("method") || !request["method"].is_string() ||
        (request.contains("id") && !(id.is_null() || id.is_string() || id.is_number_integer())))
        return rpc_error(id,-32600,"mcp.invalid_request","Richiesta JSON-RPC 2.0 non valida");
    const auto method=request["method"].get<std::string>();
    if (!request.contains("id")) return std::nullopt;
    const auto params=request.value("params",Json::object());
    if (!params.is_object()) return rpc_error(id,-32602,"mcp.invalid_params","params deve essere un oggetto JSON");
    Json result;
    try {
        if (method=="initialize")
            result={{"protocolVersion","2025-06-18"},{"capabilities",{{"tools",Json::object()},{"resources",Json::object()}}},
                    {"serverInfo",{{"name","sagomacad"},{"version","0.1.0"}}}};
        else if (method=="ping") result=Json::object();
        else if (method=="tools/list") result={{"tools",tools_catalog()}};
        else if (method=="tools/call") {
            if (!params.contains("name") || !params["name"].is_string() ||
                (params.contains("arguments") && !params["arguments"].is_object()))
                return rpc_error(id,-32602,"mcp.invalid_params","tools/call richiede name e arguments oggetto");
            result=tool(params["name"].get<std::string>(),params.value("arguments",Json::object()));
        } else if (method=="resources/list") {
            result={{"resources",Json::array({
                {{"uri","sagomacad://document"},{"name","Documento corrente"},{"mimeType","application/json"}},
                {{"uri","sagomacad://commands"},{"name","Catalogo comandi"},{"mimeType","application/json"}},
                {{"uri","sagomacad://docs/modeling-guide"},{"name","Guida modellazione"},{"mimeType","text/markdown"}}
            })}};
        } else if (method=="resources/read") {
            if (!params.contains("uri") || !params["uri"].is_string())
                return rpc_error(id,-32602,"mcp.invalid_params","resources/read richiede uri");
            const auto uri=params["uri"].get<std::string>();
            Json data;
            std::string mime="application/json";
            if (uri=="sagomacad://document") {
                const auto reply=commands_("engine.execute",{{"command","document.inspect"},{"params",Json::object()}});
                if (reply.contains("error")) return rpc_error(id,-32603,"command.failed",reply["error"].value("message","Errore del documento"));
                data=reply.at("result").at("document");
            } else if (uri=="sagomacad://commands") {
                const auto reply=commands_("engine.commands",Json::object());
                if (reply.contains("error")) return rpc_error(id,-32603,"command.failed",reply["error"].value("message","Errore del registro"));
                data=reply.at("result");
            } else if (uri=="sagomacad://docs/modeling-guide") {
                mime="text/markdown";
                data=std::string(modeling_guide);
            } else return rpc_error(id,-32602,"mcp.unknown_resource","Risorsa sconosciuta: "+uri,"Consulta resources/list");
            result={{"contents",Json::array({{{"uri",uri},{"mimeType",mime},{"text",mime=="application/json"?data.dump():data.get<std::string>()}}})}};
        } else return rpc_error(id,-32601,"mcp.unknown_method","Metodo MCP sconosciuto: "+method,"Consulta tools/list e resources/list");
    } catch (const std::exception& e) {
        return rpc_error(id,-32603,"mcp.internal_error",e.what(),"Riprova o segnala l'errore");
    }
    return Json{{"jsonrpc","2.0"},{"id",id},{"result",std::move(result)}};
}

void Server::serve(std::istream& input,std::ostream& output) {
    std::string line;
    while (std::getline(input,line)) {
        if (line.size()>1024*1024) { output<<rpc_error(nullptr,-32600,"mcp.request_too_large","Richiesta troppo grande").dump()<<'\n'; output.flush(); continue; }
        const auto request=Json::parse(line,nullptr,false);
        const auto response=request.is_discarded()
            ?std::optional<Json>(rpc_error(nullptr,-32700,"mcp.parse_error","JSON non valido"))
            :handle(request);
        if (response) { output<<response->dump()<<'\n'; output.flush(); }
    }
}
} // namespace sagomacad::automation::mcp
