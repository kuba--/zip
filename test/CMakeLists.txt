cmake_minimum_required(VERSION 2.8)

if ("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_C_COMPILER_ID}" STREQUAL "AppleClang")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -coverage -O0 -g")
endif ()

# test
include_directories(../src)
add_executable(test.exe test.c ../src/zip.c)

add_test(NAME test COMMAND test.exe)
