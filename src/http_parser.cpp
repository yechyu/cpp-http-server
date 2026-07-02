#include "http_parser.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace http {

ParseResult HttpParser::parse(const std::string& buffer) {
    const std::size_t headerEnd = buffer.find("\r\n\r\n");
    if (headerEnd == std::string::npos) return {ParseStatus::Incomplete, std::nullopt, 0};
    const std::string headerBlock = buffer.substr(0, headerEnd);
    std::istringstream stream(headerBlock);
    HttpRequest request;
    std::string requestLine;
    if (!std::getline(stream, requestLine)) return {ParseStatus::BadRequest, std::nullopt, 0};
    if (!requestLine.empty() && requestLine.back() == '\r') requestLine.pop_back();
    std::istringstream requestLineStream(requestLine);
    if (!(requestLineStream >> request.method >> request.path >> request.version)) return {ParseStatus::BadRequest, std::nullopt, 0};
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const std::size_t colon = line.find(':');
        if (colon == std::string::npos) return {ParseStatus::BadRequest, std::nullopt, 0};
        request.headers[toLower(trim(line.substr(0, colon)))] = trim(line.substr(colon + 1));
    }
    std::size_t contentLength = 0;
    auto it = request.headers.find("content-length");
    if (it != request.headers.end()) {
        try { contentLength = static_cast<std::size_t>(std::stoull(it->second)); }
        catch (...) { return {ParseStatus::BadRequest, std::nullopt, 0}; }
    }
    const std::size_t totalNeeded = headerEnd + 4 + contentLength;
    if (buffer.size() < totalNeeded) return {ParseStatus::Incomplete, std::nullopt, 0};
    request.body = buffer.substr(headerEnd + 4, contentLength);
    return {ParseStatus::Complete, request, totalNeeded};
}

std::string HttpParser::toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return s;
}

std::string HttpParser::trim(const std::string& s) {
    std::size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
    std::size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
    return s.substr(start, end - start);
}

}
