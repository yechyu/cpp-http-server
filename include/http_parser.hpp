#pragma once
#include "http_request.hpp"
#include <optional>
#include <string>

namespace http {

enum class ParseStatus { Complete, Incomplete, BadRequest };

struct ParseResult {
    ParseStatus status;
    std::optional<HttpRequest> request;
    std::size_t consumed = 0;
};

class HttpParser {
public:
    static ParseResult parse(const std::string& buffer);
private:
    static std::string toLower(std::string s);
    static std::string trim(const std::string& s);
};

}
