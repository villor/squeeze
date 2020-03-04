cmake_minimum_required(VERSION 3.10)

project(squeeze)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_library(squeeze SHARED src/squeeze.cc)
add_executable(squeeze-injector WIN32 src/injector.cc)
