#include "logger.hpp"
#include <iostream>

namespace http {

void Logger::info(const std::string& msg) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::cout << "[INFO] " << msg << '\n';
}

void Logger::request(const std::string& method, const std::string& path, int status, std::size_t bytes, std::chrono::microseconds latency) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::cout << method << ' ' << path << ' ' << status << ' ' << bytes << "B " << latency.count() << "us\n";
}

}
