project(CLOX_COPY)

cmake_minimum_required(VERSION 3.22)

include_directories(include)

file(GLOB_RECURSE SRC_LIST "src/*.c")

add_executable(clox main.c ${SRC_LIST})