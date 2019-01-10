cmake_minimum_required (VERSION 2.6)
project(slay2)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")

add_executable(slay2 main.cpp crc32.c slay2_buffer.cpp slay2_scheduler.cpp slay2.cpp slay2_nullmodem.cpp)
