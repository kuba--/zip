cmake_minimum_required(VERSION 2.8)
project(zip)

if (MSVC)
    # Use secure functions by defaualt and suppress warnings about "deprecated" functions
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /D _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /D _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT=1")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /D _CRT_NONSTDC_NO_WARNINGS=1 /D _CRT_SECURE_NO_WARNINGS=1")
endif (MSVC)

# libzip
set(SRC src/miniz.h src/zip.h src/zip.c)
add_library(${CMAKE_PROJECT_NAME} ${SRC})

# zip_test
add_executable(zip_test test/main.c)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(zip_test ${CMAKE_PROJECT_NAME})

enable_testing()
add_test(NAME zip_test COMMAND zip_test)
