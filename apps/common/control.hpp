#pragma once
#include <nlohmann/json.hpp>
#include <atomic>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

namespace sagomacad {
class ControlServer {
public:
    using Handler = std::function<nlohmann::json(const nlohmann::json&)>;
    ControlServer(int port, std::string token, Handler handler);
    ~ControlServer();
    bool start(std::string& error);
    void pump();
private:
    struct Request { nlohmann::json input; std::promise<nlohmann::json> result; };
    int port_;
    std::string token_;
    Handler handler_;
    std::atomic<bool> running_{false};
    std::intptr_t socket_ = -1;
    std::thread thread_;
    std::mutex mutex_;
    std::queue<std::shared_ptr<Request>> pending_;
    void serve();
};
}
