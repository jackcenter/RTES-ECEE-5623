cmake_minimum_required(VERSION 3.22)
project(ecen-5623-homework-3 LANGUAGES C)

include_directories(include)

add_executable(part-2-c src/sequencer.c)
target_link_libraries(part-2-c m)

add_executable(part-3-b src/deadlock.c)

add_executable(part-4-a src/posix_mq.c)
