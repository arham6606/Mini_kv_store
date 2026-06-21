cmake_minimum_required(VERSION 3.16)
project(app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(SOURCES
    src/main.cpp
    src/kv_store.cpp
    src/network/server.cpp
    src/network/socket_utils.cpp
)

add_executable(app ${SOURCES})


target_include_directories(app PRIVATE
    ${CMAKE_SOURCE_DIR}
)