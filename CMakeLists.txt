cmake_minimum_required(VERSION 3.3)
project(Shell)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror")

set(SOURCE_FILES whoosh.c)
add_executable(Shell ${SOURCE_FILES})