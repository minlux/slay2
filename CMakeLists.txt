cmake_minimum_required (VERSION 2.6)
project(slay2)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")

include_directories(src test)

add_executable(slay2 
   test/main.cpp 
   src/crc32.c 
   src/slay2_buffer.cpp 
   src/slay2_scheduler.cpp 
   src/slay2.cpp 
   src/slay2_nullmodem.cpp
)
