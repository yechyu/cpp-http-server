#pragma once
#include "logger.hpp"
#include "router.hpp"
#include "static_file.hpp"
#include <poll.h>
#include <string>
#include <vector>

namespace http {

class Server {
public:
    Server(int port, std::string staticRoot, bool quiet = false);
    void run();

private:
    struct Client {
        int fd = -1;
        std::string inBuffer;
        std::string outBuffer;
        bool closeAfterWrite = false;
    };

    int createListenSocket() const;
    void acceptClients();
    void readFromClient(std::size_t index);
    void writeToClient(std::size_t index);
    void closeClient(std::size_t index);
    void refreshPollEvents();

    [[nodiscard]] HttpResponse handleRequest(const HttpRequest& request) const;

    int port_;
    int listenFd_ = -1;
    bool quiet_ = false;

    Router router_;
    StaticFileServer staticFiles_;
    Logger logger_;

    std::vector<pollfd> pollFds_;
    std::vector<Client> clients_;
};

}
