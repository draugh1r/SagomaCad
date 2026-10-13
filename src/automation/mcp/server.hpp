#pragma once
#include <nlohmann/json.hpp>
#include <filesystem>
#include <functional>
#include <istream>
#include <optional>
#include <ostream>
#include <string_view>
#include <vector>

namespace sagomacad::automation::mcp {
using Json=nlohmann::json;
using Handler=std::function<Json(std::string_view,const Json&)>;

class Server {
public:
    Server(Handler commands,Handler visuals,bool bridge,std::vector<std::filesystem::path> roots);
    std::optional<Json> handle(const Json& request);
    void serve(std::istream& input,std::ostream& output);
private:
    Json tool(std::string_view name,const Json& arguments);
    Json invoke(std::string_view method,const Json& params);
    std::optional<std::filesystem::path> document_path(const Json& arguments,bool saving,std::string& error);
    bool in_roots(const std::filesystem::path& path) const;
    Handler commands_;
    Handler visuals_;
    bool bridge_;
    std::vector<std::filesystem::path> roots_;
};
} // namespace sagomacad::automation::mcp
