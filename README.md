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
