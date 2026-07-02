#pragma once
#include <string>
#include <unordered_map>

namespace http {

class HttpResponse {
public:
    explicit HttpResponse(int status = 200);
    HttpResponse& status(int code);
    HttpResponse& header(std::string key, std::string value);
    HttpResponse& body(std::string content);
    HttpResponse& contentType(std::string type);
    [[nodiscard]] int statusCode() const;
    [[nodiscard]] std::string serialise() const;
private:
    [[nodiscard]] static std::string reasonPhrase(int code);
    int statusCode_;
    std::unordered_map<std::string, std::string> headers_;
    std::string body_;
};

}
