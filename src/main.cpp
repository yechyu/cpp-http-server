#include "server.hpp"
#include <exception>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    int port = 8080;
    std::string staticRoot = "public";
    bool quiet = false;

    if (argc >= 2) {
        port = std::stoi(argv[1]);
    }

    if (argc >= 3) {
        staticRoot = argv[2];
    }

    for (int i = 3; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--quiet") {
            quiet = true;
        } else {
            std::cerr << "unknown argument: " << arg << '\n';
            return 1;
        }
    }

    try {
        http::Server server(port, staticRoot, quiet);
        server.run();
    } catch (const std::exception& ex) {
        std::cerr << "server error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
