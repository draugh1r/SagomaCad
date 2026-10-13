#pragma once
#include <filesystem>
#include <string>

namespace sagomacad::automation {

struct SessionFile {
    int port = 0;
    std::filesystem::path token_file;
};
std::filesystem::path default_session_file();
bool write_session_file(const std::filesystem::path& path, const SessionFile& value, std::string& error);
bool read_session_file(const std::filesystem::path& path, SessionFile& value, std::string& error);

} // namespace sagomacad::automation
