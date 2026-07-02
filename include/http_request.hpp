#pragma once
#include <string>
#include <unordered_map>

namespace http {

struct HttpRequest {
    std::string method;
    std::string path;
    std::string version;
    std::unordered_map<std::string, std::string> headers;
    std::string body;

    [[nodiscard]] bool keepAlive() const {
        auto it = headers.find("connection");
        if (it != headers.end()) return it->second != "close";
        return version == "HTTP/1.1";
    }
};

}
