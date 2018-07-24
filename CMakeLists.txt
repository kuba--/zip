cmake_minimum_required(VERSION 2.8)
project(zip)
enable_language(C)

if (MSVC)
  # Use secure functions by defaualt and suppress warnings about "deprecated" functions
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /D _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /D _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT=1")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /D _CRT_NONSTDC_NO_WARNINGS=1 /D _CRT_SECURE_NO_WARNINGS=1")
endif (MSVC)

# zip
set(SRC src/miniz.h src/zip.h src/zip.c)
add_library(${PROJECT_NAME} ${SRC})

# test
enable_testing()
add_subdirectory(test)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
find_package(Sanitizers)
add_sanitizers(${PROJECT_NAME} test.exe)
