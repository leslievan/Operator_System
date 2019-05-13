cmake_minimum_required(VERSION 3.12)
project(Operator_System_Exp3_3_fake C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
add_executable(Operator_System_Exp3_3_fake main.c common.h)