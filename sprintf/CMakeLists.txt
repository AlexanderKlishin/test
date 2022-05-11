cmake_minimum_required(VERSION 3.10)
project(sprintf)
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror")

add_executable(sprintf main.c)
#target_link_libraries(c)
