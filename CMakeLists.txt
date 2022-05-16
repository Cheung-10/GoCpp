cmake_minimum_required(VERSION 3.21)
project(GoCpp)

set(CMAKE_CXX_STANDARD 20)

add_executable(GoCpp main.cpp test.cpp goContainer/goMap.h)
