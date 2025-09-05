cmake_minimum_required(VERSION 3.24)
project(z4 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -lm")

add_executable(z4 src/z4.c src/monopoly.c)
target_include_directories(z4 PRIVATE include)