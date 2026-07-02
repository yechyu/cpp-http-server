#pragma once
#include "http_request.hpp"
#include "http_response.hpp"
#include <functional>
#include <string>
#include <unordered_map>

namespace http {

using Handler = std::function<HttpResponse(const HttpRequest&)>;

class Router {
public:
    void get(const std::string& path, Handler handler);
    void post(const std::string& path, Handler handler);
    [[nodiscard]] HttpResponse route(const HttpRequest& request) const;
private:
    [[nodiscard]] static std::string key(const std::string& method, const std::string& path);
    std::unordered_map<std::string, Handler> routes_;
};

}
