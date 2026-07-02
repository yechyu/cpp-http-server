#pragma once
#include "http_response.hpp"
#include <string>

namespace http {

class StaticFileServer {
public:
    explicit StaticFileServer(std::string root);
    [[nodiscard]] HttpResponse serve(const std::string& requestPath) const;
private:
    [[nodiscard]] static std::string contentTypeFor(const std::string& path);
    [[nodiscard]] static bool safePath(const std::string& path);
    std::string root_;
};

}
