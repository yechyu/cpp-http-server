#include "static_file.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <utility>

namespace fs = std::filesystem;

namespace http {

StaticFileServer::StaticFileServer(std::string root) : root_(std::move(root)) {}

HttpResponse StaticFileServer::serve(const std::string& requestPath) const {
    if (!safePath(requestPath)) return HttpResponse(403).contentType("text/plain").body("403 Forbidden\n");
    std::string relative = requestPath;
    if (relative == "/" || relative == "/static") relative = "/index.html";
    if (relative.rfind("/static/", 0) == 0) relative = relative.substr(std::string("/static").size());
    fs::path filePath = fs::path(root_) / relative.substr(1);
    if (!fs::exists(filePath) || !fs::is_regular_file(filePath)) return HttpResponse(404).contentType("text/plain").body("404 Not Found\n");
    std::ifstream file(filePath, std::ios::binary);
    if (!file) return HttpResponse(500).contentType("text/plain").body("500 Internal Server Error\n");
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return HttpResponse(200).contentType(contentTypeFor(filePath.string())).body(buffer.str());
}

std::string StaticFileServer::contentTypeFor(const std::string& path) {
    if (path.ends_with(".html")) return "text/html";
    if (path.ends_with(".css")) return "text/css";
    if (path.ends_with(".js")) return "application/javascript";
    if (path.ends_with(".json")) return "application/json";
    if (path.ends_with(".png")) return "image/png";
    if (path.ends_with(".jpg") || path.ends_with(".jpeg")) return "image/jpeg";
    if (path.ends_with(".txt")) return "text/plain";
    return "application/octet-stream";
}

bool StaticFileServer::safePath(const std::string& path) { return path.find("..") == std::string::npos; }

}
