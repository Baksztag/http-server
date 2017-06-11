cmake_minimum_required(VERSION 3.6)
project(http_server)

set(CMAKE_CXX_STANDARD 14)

set(SOURCE_FILES main.cpp)
add_executable(http_server ${SOURCE_FILES})
TARGET_LINK_LIBRARIES(http_server pthread)