cmake_minimum_required (VERSION 2.6)
project(slay2)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")

include_directories(src test)

add_executable(slay2_test
   test/main.cpp
   src/crc32.c
   src/slay2_buffer.cpp
   src/slay2_scheduler.cpp
   src/slay2.cpp
   src/slay2_nullmodem.cpp
)

add_executable(slay2_buffer_test
   test/slay2_buffer_test.cpp
   src/crc32.c
   src/slay2_buffer.cpp
)

add_executable(slay2_linux_test
   test/slay2_linux_test.cpp
   src/crc32.c
   src/slay2_buffer.cpp
   src/slay2_scheduler.cpp
   src/slay2.cpp
   src/slay2_linux.cpp
)
target_link_libraries(slay2_linux_test pthread)

add_executable(slay2_win32_test
   test/slay2_win32_test.cpp
   src/crc32.c
   src/slay2_buffer.cpp
   src/slay2_scheduler.cpp
   src/slay2.cpp
   src/slay2_linux.cpp
)

