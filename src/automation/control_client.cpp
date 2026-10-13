#include "automation/control_client.hpp"
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace sagomacad::automation {
namespace {
#ifdef _WIN32
using Socket = SOCKET;
void close_socket(Socket socket) { closesocket(socket); }
#else
using Socket = int;
void close_socket(Socket socket) { close(socket); }
#endif
bool send_json(Socket socket, const nlohmann::json& value) {
    const std::string bytes = value.dump() + "\n";
    std::size_t position = 0;
    while (position < bytes.size()) {
        const int sent = send(socket, bytes.data() + position, static_cast<int>(bytes.size() - position), 0);
        if (sent <= 0) return false;
        position += static_cast<std::size_t>(sent);
    }
    return true;
}
bool receive_json(Socket socket, nlohmann::json& value) {
    std::string line;
    char byte = 0;
    while (line.size() < 1024 * 1024) {
        const int read = recv(socket, &byte, 1, 0);
        if (read <= 0) return false;
        if (byte == '\n') {
            value = nlohmann::json::parse(line, nullptr, false);
            return !value.is_discarded() && value.is_object();
        }
        line.push_back(byte);
    }
    return false;
}
}

bool control_call(int port, const std::string& token, const std::string& method,
                  const nlohmann::json& params, nlohmann::json& response, std::string& error) {
    if (port < 1 || port > 65535 || token.empty() || method.empty()) {
        error = "Porta, token o metodo non validi";
        return false;
    }
#ifdef _WIN32
    WSADATA data{};
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) { error = "WSAStartup fallita"; return false; }
#endif
    const Socket socket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (socket == static_cast<Socket>(-1)) {
        error = "Impossibile creare il socket";
#ifdef _WIN32
        WSACleanup();
#endif
        return false;
    }
#ifdef _WIN32
    DWORD timeout = 5000;
#else
    timeval timeout{5, 0};
#endif
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
    setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<unsigned short>(port));
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    bool ok = false;
    if (connect(socket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0) {
        nlohmann::json auth;
        if (send_json(socket, {{"id", 0}, {"method", "auth"}, {"params", {{"token", token}}}}) &&
            receive_json(socket, auth) && auth.contains("result") &&
            auth["result"].is_object() && auth["result"].value("authenticated", false)) {
            ok = send_json(socket, {{"id", 1}, {"method", method}, {"params", params}}) &&
                 receive_json(socket, response);
            if (!ok) error = "Risposta del canale di controllo non valida";
        } else error = "Autenticazione del canale di controllo fallita";
    } else error = "Impossibile collegarsi all'app SagomaCad";
    close_socket(socket);
#ifdef _WIN32
    WSACleanup();
#endif
    return ok;
}

} // namespace sagomacad::automation
