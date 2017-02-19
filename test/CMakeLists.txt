cmake_minimum_required(VERSION 2.8)

# test
include_directories(../src)
add_executable(testzip test.c ../src/zip.c)

add_test(NAME test COMMAND testzip)
