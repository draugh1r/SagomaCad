#include "fuzz.hpp"
#include "engine/command.hpp"
#include "automation/mcp/server.hpp"
#include "automation/dispatch.hpp"
#include <iostream>
#include <limits>
#include <string>

int fuzz_commands() {
    try {
        sagomacad::engine::Engine catalog;
        std::size_t cases = 0;
        std::size_t rejected = 0;
        std::size_t valid_empty = 0;
        std::size_t mcp_cases = 0;
        sagomacad::engine::Engine mcp_engine;
        sagomacad::automation::mcp::Server mcp_server(
            [&](std::string_view method,const nlohmann::json& params) {
                return sagomacad::automation::dispatch(mcp_engine,method,params);
            },
            [](std::string_view,const nlohmann::json&) {return nlohmann::json::object();},
            false,{});
        const auto mcp_call=[&](const nlohmann::json& arguments) {
            const auto reply=mcp_server.handle({{"jsonrpc","2.0"},{"id",1},{"method","tools/call"},
                {"params",{{"name","command_run"},{"arguments",arguments}}}});
            ++mcp_cases;
            if (!reply || !reply->contains("result") || !(*reply)["result"].value("isError",false))
                throw std::runtime_error("Fuzz MCP: input non valido accettato");
        };
        for (const auto& spec : catalog.registry().specs()) {
            {
                sagomacad::engine::Engine engine;
                const auto result = engine.execute(spec.id, nlohmann::json::object());
                ++cases;
                if (result.ok) ++valid_empty;
                else { ++rejected; if (result.error.empty()) throw std::runtime_error("errore vuoto"); }
            }
            const auto nan = std::numeric_limits<double>::quiet_NaN();
            const nlohmann::json wrong_root = nlohmann::json::array({1, "test"});
            const nlohmann::json wrong_types = {{"name", 12}, {"path", false},
                                                 {"expression", nlohmann::json::array()}, {"unit", 3}};
            const nlohmann::json huge = {{"name", std::string(10000, 'x')},
                                         {"path", std::string(10000, 'x')},
                                         {"expression", "1e309 mm"},
                                         {"unit", "mm"}};
            const nlohmann::json nonfinite = {{"name", nan}, {"path", nan},
                                              {"expression", nan}, {"unit", nan}};
            for (const auto& params : {wrong_root, wrong_types, huge, nonfinite}) {
                sagomacad::engine::Engine engine;
                const auto result = engine.execute(spec.id, params);
                ++cases;
                if (result.ok || result.error.empty()) {
                    std::cerr << "Fuzz: input non valido accettato da " << spec.id << '\n';
                    return 1;
                }
                ++rejected;
                mcp_call({{"command",spec.id},{"params",params}});
            }
            sagomacad::engine::Engine engine;
            const auto missing = engine.execute(spec.id + ".inesistente");
            ++cases;
            if (missing.ok || missing.error.empty()) {
                std::cerr << "Fuzz: ID inesistente accettato\n";
                return 1;
            }
            ++rejected;
            mcp_call({{"command",spec.id+".inesistente"}});
        }
        for (const auto& malformed : {nlohmann::json{{"command",7}},nlohmann::json{{"params",false}}})
            mcp_call(malformed);
        for (const auto& malformed : {nlohmann::json(nullptr),nlohmann::json::array({1})}) {
            const auto reply=mcp_server.handle({{"jsonrpc","2.0"},{"id",1},{"method","tools/call"},
                {"params",{{"name","command_run"},{"arguments",malformed}}}});
            ++mcp_cases;
            if (!reply || !reply->contains("error") || (*reply)["error"].value("code",0)!=-32602)
                throw std::runtime_error("Fuzz MCP: errore JSON-RPC mancante");
        }
        std::cout << nlohmann::json({{"commands", catalog.registry().specs().size()},
                                    {"cases", cases}, {"rejected", rejected},
                                    {"valid_empty", valid_empty},{"mcp_cases",mcp_cases}}).dump() << '\n';
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fuzz interrotto: " << e.what() << '\n';
        return 1;
    }
}
