cmake_minimum_required(VERSION 2.8)

if ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_C_COMPILER_ID}" STREQUAL "AppleClang")
  if(ENABLE_COVERAGE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g ")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
  endif()
endif ()

# test
include_directories(../src)

set(test_out test.out)
set(test_miniz_out test_miniz.out)

if (WIN32)
  set(test_out test.exe)
  set(test_miniz_out test_miniz.exe)
endif()

add_executable(${test_out} test.c ../src/zip.c)
add_executable(${test_miniz_out} test_miniz.c)

add_test(NAME ${test_out} COMMAND ${test_out})
add_test(NAME ${test_miniz_out} COMMAND ${test_miniz_out})