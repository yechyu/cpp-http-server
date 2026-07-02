#include "http_response.hpp"
#include <sstream>
#include <utility>

namespace http {

HttpResponse::HttpResponse(int status) : statusCode_(status) {}

HttpResponse& HttpResponse::status(int code) { statusCode_ = code; return *this; }
HttpResponse& HttpResponse::header(std::string key, std::string value) { headers_[std::move(key)] = std::move(value); return *this; }
HttpResponse& HttpResponse::body(std::string content) { body_ = std::move(content); return *this; }
HttpResponse& HttpResponse::contentType(std::string type) { headers_["Content-Type"] = std::move(type); return *this; }
int HttpResponse::statusCode() const { return statusCode_; }

std::string HttpResponse::serialise() const {
    std::ostringstream out;
    out << "HTTP/1.1 " << statusCode_ << ' ' << reasonPhrase(statusCode_) << "\r\n";
    out << "Content-Length: " << body_.size() << "\r\n";
    for (const auto& [key, value] : headers_) out << key << ": " << value << "\r\n";
    out << "\r\n" << body_;
    return out.str();
}

std::string HttpResponse::reasonPhrase(int code) {
    switch (code) {
        case 200: return "OK";
        case 201: return "Created";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 500: return "Internal Server Error";
        default: return "Unknown";
    }
}

}
