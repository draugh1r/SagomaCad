#include "automation/session_file.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <system_error>
#ifdef _WIN32
#include <windows.h>
#endif

namespace sagomacad::automation {

std::filesystem::path default_session_file() {
    return std::filesystem::current_path() / ".sagomacad" / "session.json";
}

bool write_session_file(const std::filesystem::path& path, const SessionFile& value, std::string& error) {
    try {
        if (value.port < 1 || value.port > 65535 || value.token_file.empty())
            throw std::runtime_error("Dati della sessione non validi");
        std::filesystem::create_directories(path.parent_path());
        std::filesystem::permissions(path.parent_path(), std::filesystem::perms::owner_all,
                                     std::filesystem::perm_options::replace);
        auto temporary = path;
        temporary += ".tmp";
        const auto token_utf8 = value.token_file.u8string();
        {
            std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
            if (!output) throw std::runtime_error("Impossibile scrivere il file di sessione");
            output << nlohmann::json({{"port", value.port},
                                      {"token_file", std::string(token_utf8.begin(), token_utf8.end())}}).dump(2);
            output.flush();
            if (!output) throw std::runtime_error("Scrittura del file di sessione incompleta");
        }
        std::filesystem::permissions(temporary,
                                     std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
                                     std::filesystem::perm_options::replace);
#ifdef _WIN32
        if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            throw std::system_error(GetLastError(), std::system_category(), "Impossibile pubblicare la sessione");
#else
        std::filesystem::rename(temporary, path);
#endif
        error.clear();
        return true;
    } catch (const std::exception& e) {
        error = e.what();
        return false;
    }
}

bool read_session_file(const std::filesystem::path& path, SessionFile& value, std::string& error) {
    try {
        std::ifstream input(path, std::ios::binary);
        if (!input) throw std::runtime_error("File di sessione assente: " + path.string());
        const auto data = nlohmann::json::parse(input);
        if (!data.is_object() || !data.contains("port") || !data["port"].is_number_integer() ||
            !data.contains("token_file") || !data["token_file"].is_string())
            throw std::runtime_error("File di sessione non valido");
        value.port = data["port"].get<int>();
        value.token_file = std::filesystem::u8path(data["token_file"].get<std::string>());
        if (value.port < 1 || value.port > 65535 || value.token_file.empty())
            throw std::runtime_error("File di sessione non valido");
        error.clear();
        return true;
    } catch (const std::exception& e) {
        error = e.what();
        return false;
    }
}

} // namespace sagomacad::automation
