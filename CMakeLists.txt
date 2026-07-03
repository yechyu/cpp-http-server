cmake_minimum_required(VERSION 3.16)
project(cpp_http_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Threads REQUIRED)

add_compile_options(-Wall -Wextra -Wpedantic)

include_directories(include)

add_executable(http_server
    src/http_parser.cpp
    src/http_response.cpp
    src/router.cpp
    src/static_file.cpp
    src/logger.cpp
    src/server.cpp
    src/main.cpp
)

target_link_libraries(http_server PRIVATE Threads::Threads)

add_executable(http_bench
    bench/http_bench.cpp
)

target_link_libraries(http_bench PRIVATE Threads::Threads)
