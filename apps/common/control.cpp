#include "control.hpp"
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif
#include <chrono>

namespace sagomacad {
namespace {
#ifdef _WIN32
using Socket = SOCKET;
void close_socket(Socket s) { closesocket(s); }
#else
using Socket = int;
void close_socket(Socket s) { close(s); }
#endif
nlohmann::json failure(const nlohmann::json& id, const std::string& msg) {
    return {{"id",id},{"error",{{"message",msg}}}};
}
bool send_line(Socket client, const nlohmann::json& reply) {
    const std::string line=reply.dump()+"\n";
    size_t offset=0;
    while (offset<line.size()) {
        const int sent=send(client,line.data()+offset,static_cast<int>(line.size()-offset),0);
        if (sent<=0) return false;
        offset+=static_cast<size_t>(sent);
    }
    return true;
}
}
ControlServer::ControlServer(int port,std::string token,Handler handler)
    : port_(port),token_(std::move(token)),handler_(std::move(handler)) {}
ControlServer::~ControlServer() {
    running_=false;
    if (socket_!=-1) {
#ifdef _WIN32
        shutdown(static_cast<Socket>(socket_),SD_BOTH);
#else
        shutdown(static_cast<Socket>(socket_),SHUT_RDWR);
#endif
        close_socket(static_cast<Socket>(socket_));
    }
    if (thread_.joinable()) thread_.join();
#ifdef _WIN32
    WSACleanup();
#endif
}
bool ControlServer::start(std::string& error) {
    if (port_<1 || port_>65535 || token_.empty()) { error="invalid port or empty token"; return false; }
#ifdef _WIN32
    WSADATA data{};
    if (WSAStartup(MAKEWORD(2,2),&data)!=0) { error="WSAStartup failed"; return false; }
#endif
    Socket sock=socket(AF_INET,SOCK_STREAM,0);
    if (sock==static_cast<Socket>(-1)) { error="cannot create socket"; return false; }
    int reuse=1; setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,reinterpret_cast<const char*>(&reuse),sizeof(reuse));
    sockaddr_in addr{}; addr.sin_family=AF_INET; addr.sin_port=htons(static_cast<unsigned short>(port_));
    addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    if (bind(sock,reinterpret_cast<sockaddr*>(&addr),sizeof(addr))!=0 || listen(sock,4)!=0) {
        error="cannot bind loopback port"; close_socket(sock); return false;
    }
    socket_=static_cast<std::intptr_t>(sock); running_=true;
    thread_=std::thread(&ControlServer::serve,this);
    return true;
}
void ControlServer::pump() {
    std::queue<std::shared_ptr<Request>> work;
    { std::lock_guard lock(mutex_); std::swap(work,pending_); }
    while (!work.empty()) {
        auto request=work.front(); work.pop();
        try { request->result.set_value(handler_(request->input)); }
        catch (const std::exception& e) { request->result.set_value(failure(request->input.value("id",nlohmann::json()),e.what())); }
    }
}
void ControlServer::serve() {
    while (running_) {
        Socket client=accept(static_cast<Socket>(socket_),nullptr,nullptr);
        if (client==static_cast<Socket>(-1)) break;
#ifdef _WIN32
        DWORD timeout=5000;
#else
        timeval timeout{5,0};
#endif
        setsockopt(client,SOL_SOCKET,SO_RCVTIMEO,reinterpret_cast<const char*>(&timeout),sizeof(timeout));
        bool authed=false;
        std::string buffer;
        char chunk[4096];
        while (running_) {
            int n=recv(client,chunk,sizeof(chunk),0);
            if (n<=0) break;
            buffer.append(chunk,static_cast<size_t>(n));
            if (buffer.size()>1024*1024) { send_line(client,failure(nullptr,"message too large")); break; }
            size_t eol;
            while ((eol=buffer.find('\n'))!=std::string::npos) {
                std::string line=buffer.substr(0,eol); buffer.erase(0,eol+1);
                auto input=nlohmann::json::parse(line,nullptr,false);
                if (input.is_discarded() || !input.is_object()) { if (!send_line(client,failure(nullptr,"invalid JSON"))) break; continue; }
                auto id=input.value("id",nlohmann::json());
                if (!input.contains("method") || !input["method"].is_string()) {
                    if (!send_line(client,failure(id,"method must be a string"))) break;
                    continue;
                }
                auto method=input["method"].get<std::string>();
                if (!authed) {
                    const auto params=input.value("params",nlohmann::json::object());
                    if (method=="auth" && params.is_object() && params.contains("token") &&
                        params["token"].is_string() && params["token"].get<std::string>()==token_) {
                        authed=true; send_line(client,{{"id",id},{"result",{{"authenticated",true}}}});
                    } else { send_line(client,failure(id,"authentication required")); }
                    continue;
                }
                auto req=std::make_shared<Request>(); req->input=std::move(input);
                auto response=req->result.get_future();
                { std::lock_guard lock(mutex_); pending_.push(req); }
                while (running_ && response.wait_for(std::chrono::milliseconds(100))!=std::future_status::ready) {}
                if (!running_) break;
                if (!send_line(client,response.get())) break;
            }
        }
        close_socket(client);
    }
}
}
