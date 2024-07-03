cmake_minimum_required(VERSION 3.22)
project(ecen-5623-homework-3 LANGUAGES C)

include_directories(include)



add_executable(${PROJECT_NAME} src/sequencer.c)
target_link_libraries(${PROJECT_NAME} m)

add_executable(time_test src/timer_test.c)