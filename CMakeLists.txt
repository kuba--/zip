cmake_minimum_required(VERSION 2.8)
project(zip)

set(SRC src/miniz.h src/zip.h src/zip.c)

add_library(${CMAKE_PROJECT_NAME} ${SRC})
