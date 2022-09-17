cmake_minimum_required(VERSION 3.5)

project(script-engines LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(Z_FORTH
    zForth/src/zforth/zforth.c
    zForth/src/zforth/zforth.h
    zfconf.h
)

add_executable(script-engines main.cpp ${Z_FORTH})

include_directories(zForth/src/zforth ${CMAKE_CURRENT_SOURCE_DIR})

