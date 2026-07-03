# HTTP/1.1 Server from Scratch in C++20

A systems-level HTTP server implemented from scratch in C++20.

## Features

- Manual HTTP/1.1 request parsing
- `GET` and `POST` support
- Static file serving from `public/`
- Basic routing
- Non-blocking sockets
- Event-driven server loop using `poll`
- Multiple concurrent clients
- Keep-alive support for HTTP/1.1
- Optional structured request logging
- High-throughput benchmark client with persistent connections, concurrency, and HTTP pipelining

## Build without CMake

```bash
g++ -std=c++20 src/*.cpp -Iinclude -O3 -march=native -pthread -o http_server
g++ -std=c++20 bench/http_bench.cpp -O3 -march=native -pthread -o http_bench
```

## Build with CMake

```bash
mkdir -p build
cd build
cmake ..
make
```

## Run the server

For benchmarking, use `--quiet` so terminal logging does not dominate the result.

```bash
./http_server 8080 public --quiet
```

For development logging:

```bash
./http_server 8080 public
```

## Test manually

Open another terminal:

```bash
curl http://localhost:8080/
curl http://localhost:8080/health
curl http://localhost:8080/static/index.html
curl -X POST http://localhost:8080/echo -d "hello from curl"
```

## Benchmark

Arguments:

```bash
./http_bench <host> <port> <path> <requests> <concurrency> <pipeline_depth>
```

Example:

```bash
./http_bench 127.0.0.1 8080 /health 100000 4 32
```

The upgraded benchmark client keeps connections open, sends pipelined HTTP requests, and uses multiple worker threads. This removes most TCP connection setup overhead and better measures the server request path.

Example result from a local run in this environment:

```text
Requests: 100000
Success: 100000
Failed: 0
Concurrency: 4
Pipeline depth: 32
Elapsed: 0.713104 s
Throughput: 140232 req/sec
```

Actual performance depends on machine, compiler, OS, and background load.

## Resume bullet

Built a C++20 HTTP/1.1 server from scratch using non-blocking sockets and an event-driven poll loop, implementing manual request parsing, static file serving, routing, keep-alive connections, structured logging, and a pipelined benchmark client reaching 100k+ requests/sec locally.
