#include <arpa/inet.h>
#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace {

struct Config {
    std::string host = "127.0.0.1";
    int port = 8080;
    std::string path = "/health";
    int requests = 100000;
    int concurrency = 4;
    int pipelineDepth = 32;
};

struct WorkerResult {
    int attempted = 0;
    int success = 0;
    int failed = 0;
};

void setTcpNoDelay(int fd) {
    int yes = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes));

#ifdef SO_NOSIGPIPE
    setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &yes, sizeof(yes));
#endif
}

int connectTo(const std::string& host, int port) {
    const int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        throw std::runtime_error("socket failed");
    }

    setTcpNoDelay(fd);

    sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<std::uint16_t>(port));

    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1) {
        close(fd);
        throw std::runtime_error("inet_pton failed");
    }

    if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
        close(fd);
        throw std::runtime_error("connect failed");
    }

    return fd;
}

void sendAll(int fd, const std::string& data) {
    std::size_t sent = 0;

    while (sent < data.size()) {
        const ssize_t n = send(fd, data.data() + sent, data.size() - sent, 0);
        if (n <= 0) {
            throw std::runtime_error("send failed");
        }
        sent += static_cast<std::size_t>(n);
    }
}

std::size_t parseContentLength(const std::string& headers) {
    const std::string key = "Content-Length:";
    std::size_t pos = headers.find(key);
    if (pos == std::string::npos) {
        return 0;
    }

    pos += key.size();
    while (pos < headers.size() && headers[pos] == ' ') {
        ++pos;
    }

    const std::size_t end = headers.find("\r\n", pos);
    return static_cast<std::size_t>(std::stoull(headers.substr(pos, end - pos)));
}

bool readOneResponse(int fd, std::string& buffer) {
    char temp[8192];

    while (true) {
        const std::size_t headerEnd = buffer.find("\r\n\r\n");
        if (headerEnd != std::string::npos) {
            const std::string headers = buffer.substr(0, headerEnd + 2);
            const std::size_t bodyLength = parseContentLength(headers);
            const std::size_t totalLength = headerEnd + 4 + bodyLength;

            if (buffer.size() >= totalLength) {
                const bool ok = buffer.rfind("HTTP/1.1 200", 0) == 0;
                buffer.erase(0, totalLength);
                return ok;
            }
        }

        const ssize_t n = recv(fd, temp, sizeof(temp), 0);
        if (n <= 0) {
            return false;
        }
        buffer.append(temp, static_cast<std::size_t>(n));
    }
}

WorkerResult runWorker(const Config& config, int requestsForWorker) {
    WorkerResult result;
    result.attempted = requestsForWorker;

    const int fd = connectTo(config.host, config.port);

    const std::string request =
        "GET " + config.path + " HTTP/1.1\r\n"
        "Host: " + config.host + "\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";

    std::string responseBuffer;
    responseBuffer.reserve(64 * 1024);

    int completed = 0;
    while (completed < requestsForWorker) {
        const int batch = std::min(config.pipelineDepth, requestsForWorker - completed);

        std::string burst;
        burst.reserve(request.size() * static_cast<std::size_t>(batch));
        for (int i = 0; i < batch; ++i) {
            burst += request;
        }

        try {
            sendAll(fd, burst);
        } catch (...) {
            result.failed += requestsForWorker - completed;
            close(fd);
            return result;
        }

        for (int i = 0; i < batch; ++i) {
            if (readOneResponse(fd, responseBuffer)) {
                ++result.success;
            } else {
                ++result.failed;
            }
            ++completed;
        }
    }

    close(fd);
    return result;
}

Config parseArgs(int argc, char** argv) {
    Config config;

    if (argc >= 2) config.host = argv[1];
    if (argc >= 3) config.port = std::stoi(argv[2]);
    if (argc >= 4) config.path = argv[3];
    if (argc >= 5) config.requests = std::stoi(argv[4]);
    if (argc >= 6) config.concurrency = std::stoi(argv[5]);
    if (argc >= 7) config.pipelineDepth = std::stoi(argv[6]);

    if (config.requests <= 0) throw std::invalid_argument("requests must be positive");
    if (config.concurrency <= 0) throw std::invalid_argument("concurrency must be positive");
    if (config.pipelineDepth <= 0) throw std::invalid_argument("pipeline depth must be positive");

    return config;
}

} // namespace

int main(int argc, char** argv) {
    try {
        const Config config = parseArgs(argc, argv);

        std::vector<std::thread> threads;
        std::vector<WorkerResult> results(static_cast<std::size_t>(config.concurrency));

        const auto start = std::chrono::steady_clock::now();

        for (int i = 0; i < config.concurrency; ++i) {
            const int base = config.requests / config.concurrency;
            const int extra = i < (config.requests % config.concurrency) ? 1 : 0;
            const int workerRequests = base + extra;

            threads.emplace_back([&, i, workerRequests] {
                results[static_cast<std::size_t>(i)] = runWorker(config, workerRequests);
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        const auto end = std::chrono::steady_clock::now();
        const double seconds = std::chrono::duration<double>(end - start).count();

        int attempted = 0;
        int success = 0;
        int failed = 0;

        for (const auto& result : results) {
            attempted += result.attempted;
            success += result.success;
            failed += result.failed;
        }

        std::cout << "Requests: " << attempted << '\n';
        std::cout << "Success: " << success << '\n';
        std::cout << "Failed: " << failed << '\n';
        std::cout << "Concurrency: " << config.concurrency << '\n';
        std::cout << "Pipeline depth: " << config.pipelineDepth << '\n';
        std::cout << "Elapsed: " << seconds << " s\n";
        std::cout << "Throughput: " << static_cast<double>(success) / seconds << " req/sec\n";

    } catch (const std::exception& ex) {
        std::cerr << "benchmark error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
