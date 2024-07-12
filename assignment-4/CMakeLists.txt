cmake_minimum_required(VERSION 3.22)
project(ecen-5623-homework-4 LANGUAGES C)

include_directories(include)

add_executable(capture src/capture.c)
add_executable(capture-fifo src/capture-fifo.c)

add_library(${PROJECT_NAME}_lib
    lib/utilities.c
)

target_link_libraries(capture ${PROJECT_NAME}_lib)
target_link_libraries(capture-fifo ${PROJECT_NAME}_lib)
