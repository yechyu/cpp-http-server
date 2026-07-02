#pragma once
#include <chrono>
#include <mutex>
#include <string>

namespace http {

class Logger {
public:
    void info(const std::string& msg);
    void request(const std::string& method, const std::string& path, int status, std::size_t bytes, std::chrono::microseconds latency);
private:
    std::mutex mutex_;
};

}
