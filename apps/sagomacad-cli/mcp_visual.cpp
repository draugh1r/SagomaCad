#include "mcp_visual.hpp"
#include "shell.hpp"
#include "doc/document.hpp"
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace {
namespace fs=std::filesystem;
using Json=nlohmann::json;
Json failure(std::string code,std::string message,std::string suggestion={}) {
    Json data={{"code",std::move(code)},{"message",std::move(message)},{"suggestion",std::move(suggestion)}};
    return {{"isError",true},{"content",Json::array({{{"type","text"},{"text",data.dump()}}})},
            {"structuredContent",data}};
}
std::string base64(const std::vector<unsigned char>& bytes) {
    static constexpr char alphabet[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    result.reserve((bytes.size()+2)/3*4);
    for (std::size_t i=0;i<bytes.size();i+=3) {
        const unsigned a=bytes[i],b=i+1<bytes.size()?bytes[i+1]:0,c=i+2<bytes.size()?bytes[i+2]:0;
        result.push_back(alphabet[a>>2]); result.push_back(alphabet[((a&3)<<4)|(b>>4)]);
        result.push_back(i+1<bytes.size()?alphabet[((b&15)<<2)|(c>>6)]:'=');
        result.push_back(i+2<bytes.size()?alphabet[c&63]:'=');
    }
    return result;
}
Json image_result(const fs::path& path,const Json& metadata) {
    std::ifstream input(path,std::ios::binary);
    if (!input) return failure("mcp.image_missing","Impossibile leggere il PNG prodotto");
    std::vector<unsigned char> bytes(std::istreambuf_iterator<char>{input},{});
    if (bytes.size()<24 || bytes.size()>12*1024*1024 ||
        std::string_view(reinterpret_cast<const char*>(bytes.data()),8)!=std::string_view("\x89PNG\r\n\x1a\n",8))
        return failure("mcp.invalid_image","Il rendering non ha prodotto un PNG valido");
    return {{"content",Json::array({{{"type","image"},{"mimeType","image/png"},{"data",base64(bytes)}}})},
            {"structuredContent",metadata}};
}
fs::path temporary_png() {
    static std::atomic<unsigned> serial{0};
    const auto stamp=std::chrono::steady_clock::now().time_since_epoch().count();
    return fs::temp_directory_path()/("sagomacad-mcp-"+std::to_string(stamp)+"-"+std::to_string(++serial)+".png");
}
}

Json mcp_visual_tool(std::string_view name,const Json& a,const McpInvoke& commands,
                     bool bridge,sagomacad::engine::Engine& local_engine) {
    if (name=="render_view") {
        if (!a.is_object() || !a.contains("view") || !a["view"].is_string() ||
            (a.contains("size") && !a["size"].is_string()))
            return failure("mcp.invalid_arguments","render_view richiede view e size stringa opzionale");
        for (auto it=a.begin();it!=a.end();++it)
            if (it.key()!="view" && it.key()!="size") return failure("mcp.invalid_arguments","Argomento sconosciuto: "+it.key());
        const auto view=a["view"].get<std::string>();
        if (view!="iso" && view!="top" && view!="front" && view!="right")
            return failure("mcp.invalid_view","Vista sconosciuta","Usa iso, top, front o right");
        int width=800,height=600;
        if (a.contains("size")) {
            const auto size=a["size"].get<std::string>();
            const auto x=size.find('x');
            if (x==std::string::npos) return failure("mcp.invalid_size","size deve essere WIDTHxHEIGHT");
            try {
                std::size_t used_w=0,used_h=0;
                width=std::stoi(size.substr(0,x),&used_w);
                height=std::stoi(size.substr(x+1),&used_h);
                if (used_w!=x || used_h!=size.size()-x-1) throw std::invalid_argument("size");
            } catch (...) { return failure("mcp.invalid_size","size deve essere WIDTHxHEIGHT"); }
        }
        if (width<320 || height<240 || width>1600 || height>1200 || width*height>1800000)
            return failure("mcp.invalid_size","Immagine fuori intervallo (320x240–1600x1200)");
        sagomacad::engine::Engine copied_engine;
        sagomacad::engine::Engine* render_engine=&local_engine;
        if (bridge) {
            const auto reply=commands("engine.execute",{{"command","document.inspect"},{"params",Json::object()}});
            if (!reply.is_object() || reply.contains("error") || !reply.contains("result"))
                return failure("mcp.bridge_failed","Impossibile leggere il documento dall'app");
            std::string error;
            if (!sagomacad::doc::from_json(reply["result"].at("document"),copied_engine.session().document,error))
                return failure("mcp.invalid_document",error);
            render_engine=&copied_engine;
        }
        sagomacad::Shell shell(width+320,height+230,1,true,true);
        if (!shell.valid()) return failure("mcp.render_failed",shell.error());
        shell.bind_engine(render_engine);
        if (view=="top") shell.state().camera.look_from({0,0,1});
        else if (view=="front") shell.state().camera.look_from({0,-1,0});
        else if (view=="right") shell.state().camera.look_from({1,0,0});
        for (int i=0;i<3;++i) { shell.poll(); shell.render(); }
        const auto path=temporary_png();
        std::string error;
        if (!shell.screenshot(path.string(),error,true,width,height)) return failure("mcp.render_failed",error);
        auto result=image_result(path,{{"view",view},{"width",width},{"height",height}});
        std::error_code ignored; fs::remove(path,ignored);
        return result;
    }
    if (name=="ui_screenshot") {
        if (!a.is_object() || !a.empty()) return failure("mcp.invalid_arguments","ui_screenshot non accetta argomenti");
        if (!bridge) return failure("mcp.bridge_required","ui_screenshot richiede l'app aperta");
        const auto path=temporary_png();
        const auto filename=path.filename().string();
        const auto reply=commands("ui.screenshot",{{"path",filename}});
        if (!reply.is_object() || reply.contains("error") || !reply.contains("result"))
            return failure("mcp.screenshot_failed",reply.contains("error")?reply["error"].value("message","Screenshot fallito"):"Screenshot fallito");
        const auto saved=fs::u8path(reply["result"].at("path").get<std::string>());
        auto result=image_result(saved,{{"path",saved.string()}});
        std::error_code ignored; fs::remove(saved,ignored);
        return result;
    }
    return failure("mcp.unknown_tool","Tool visivo sconosciuto");
}
