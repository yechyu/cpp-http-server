#include "router.hpp"
#include <utility>

namespace http {

void Router::get(const std::string& path, Handler handler) { routes_[key("GET", path)] = std::move(handler); }
void Router::post(const std::string& path, Handler handler) { routes_[key("POST", path)] = std::move(handler); }

HttpResponse Router::route(const HttpRequest& request) const {
    auto it = routes_.find(key(request.method, request.path));
    if (it == routes_.end()) return HttpResponse(404).contentType("text/plain").body("404 Not Found\n");
    return it->second(request);
}

std::string Router::key(const std::string& method, const std::string& path) { return method + " " + path; }

}
