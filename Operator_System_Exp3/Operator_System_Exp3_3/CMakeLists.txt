cmake_minimum_required(VERSION 3.12)
project(exp3_3 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")

add_executable(exp3_3 main.c common.h)