#include "server.hpp"
#include "http_parser.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

namespace http {
namespace {
void setNonBlocking(int fd) {
    const int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) throw std::runtime_error("fcntl(F_GETFL) failed");
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) throw std::runtime_error("fcntl(F_SETFL) failed");
}

void setTcpNoDelay(int fd) {
    int yes = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes));
}
}

Server::Server(int port, std::string staticRoot, bool quiet)
    : port_(port), quiet_(quiet), staticFiles_(std::move(staticRoot)) {
    router_.get("/health", [](const HttpRequest&) { return HttpResponse(200).contentType("application/json").body("{\"status\":\"ok\"}\n"); });
    router_.post("/echo", [](const HttpRequest& request) { return HttpResponse(200).contentType("text/plain").body(request.body); });
}

void Server::run() {
    listenFd_ = createListenSocket();
    pollFds_.push_back({listenFd_, POLLIN, 0});
    if (!quiet_) logger_.info("listening on port " + std::to_string(port_));
    while (true) {
        refreshPollEvents();
        const int ready = poll(pollFds_.data(), pollFds_.size(), -1);
        (void) ready;
        if (ready == -1) {
            if (errno == EINTR) continue;
            throw std::runtime_error("poll failed");
        }
        if (pollFds_[0].revents & POLLIN) acceptClients();
        for (std::size_t i = 1; i < pollFds_.size();) {
            const std::size_t before = pollFds_.size();
            if (pollFds_[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                closeClient(i);
            } else {
                if (pollFds_[i].revents & POLLIN) readFromClient(i);
                if (i < pollFds_.size() && (pollFds_[i].revents & POLLOUT)) writeToClient(i);
            }
            if (pollFds_.size() == before) ++i;
        }
    }
}

int Server::createListenSocket() const {
    const int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) throw std::runtime_error("socket failed");
    int yes = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) { close(fd); throw std::runtime_error("setsockopt failed"); }
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<std::uint16_t>(port_));
    if (bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) { close(fd); throw std::runtime_error("bind failed"); }
    if (listen(fd, SOMAXCONN) == -1) { close(fd); throw std::runtime_error("listen failed"); }
    setNonBlocking(fd);
    return fd;
}

void Server::acceptClients() {
    while (true) {
        sockaddr_in clientAddr{};
        socklen_t len = sizeof(clientAddr);
        const int fd = accept(listenFd_, reinterpret_cast<sockaddr*>(&clientAddr), &len);
        if (fd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            throw std::runtime_error("accept failed");
        }
        setNonBlocking(fd);
        setTcpNoDelay(fd);
        clients_.push_back(Client{.fd = fd});
        pollFds_.push_back({fd, POLLIN, 0});
    }
}

void Server::readFromClient(std::size_t index) {
    Client& client = clients_[index - 1];
    char buffer[8192];
    while (true) {
        const ssize_t n = recv(client.fd, buffer, sizeof(buffer), 0);
        if (n > 0) client.inBuffer.append(buffer, static_cast<std::size_t>(n));
        else if (n == 0) { closeClient(index); return; }
        else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            closeClient(index); return;
        }
    }
    while (true) {
        const auto start = std::chrono::steady_clock::now();
        ParseResult result = HttpParser::parse(client.inBuffer);
        if (result.status == ParseStatus::Incomplete) break;
        if (result.status == ParseStatus::BadRequest) {
            HttpResponse bad(400);
            bad.contentType("text/plain").body("400 Bad Request\n").header("Connection", "close");
            client.outBuffer += bad.serialise();
            client.closeAfterWrite = true;
            client.inBuffer.clear();
            break;
        }
        HttpRequest request = std::move(*result.request);
        client.inBuffer.erase(0, result.consumed);
        HttpResponse response = handleRequest(request);
        if (request.keepAlive()) response.header("Connection", "keep-alive");
        else { response.header("Connection", "close"); client.closeAfterWrite = true; }
        std::string serialised = response.serialise();
        const auto end = std::chrono::steady_clock::now();
        const auto latency = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        if (!quiet_) logger_.request(request.method, request.path, response.statusCode(), serialised.size(), latency);
        client.outBuffer += serialised;
    }
}

void Server::writeToClient(std::size_t index) {
    Client& client = clients_[index - 1];
    while (!client.outBuffer.empty()) {
        const ssize_t n = send(client.fd, client.outBuffer.data(), client.outBuffer.size(), 0);
        if (n > 0) client.outBuffer.erase(0, static_cast<std::size_t>(n));
        else if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) break;
        else { closeClient(index); return; }
    }
    if (client.outBuffer.empty() && client.closeAfterWrite) closeClient(index);
}

void Server::closeClient(std::size_t index) {
    if (index == 0 || index >= pollFds_.size()) return;
    const std::size_t clientIndex = index - 1;
    close(clients_[clientIndex].fd);
    clients_.erase(clients_.begin() + static_cast<std::ptrdiff_t>(clientIndex));
    pollFds_.erase(pollFds_.begin() + static_cast<std::ptrdiff_t>(index));
}

void Server::refreshPollEvents() {
    pollFds_[0].events = POLLIN;
    pollFds_[0].revents = 0;
    for (std::size_t i = 1; i < pollFds_.size(); ++i) {
        Client& client = clients_[i - 1];
        pollFds_[i].events = POLLIN;
        if (!client.outBuffer.empty()) pollFds_[i].events |= POLLOUT;
        pollFds_[i].revents = 0;
    }
}

HttpResponse Server::handleRequest(const HttpRequest& request) const {
    if (request.method == "GET" && (request.path == "/" || request.path.rfind("/static", 0) == 0)) return staticFiles_.serve(request.path);
    if (request.method != "GET" && request.method != "POST") return HttpResponse(405).contentType("text/plain").body("405 Method Not Allowed\n");
    return router_.route(request);
}

}
