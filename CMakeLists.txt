cmake_minimum_required(VERSION 3.24)
project(webServer)

set(CMAKE_CXX_STANDARD 17)

add_executable(webServer main.cpp locker.h threadpool.h timer.h http_conn.h timer.cpp http_conn.cpp)
