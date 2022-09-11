cmake_minimum_required(VERSION 3.5)

project(script-engines LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(script-engines main.cpp)

install(TARGETS script-engines
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
