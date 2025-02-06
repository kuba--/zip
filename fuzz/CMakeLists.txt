# Utilized by OSSFuzz to build the harness(es) for continuous fuzz-testing
# OSSFuzz defines the following environment variables, that this target relies upon:
# CXX, CFLAGS, LIB_FUZZING_ENGINE, OUT

set(CMAKE_C_STANDARD 23)

add_definitions(-DNDEBUG)  # Do not want assertions

if (DEFINED ENV{CFLAGS})
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} $ENV{CFLAGS}")
endif ()


add_executable(zip_stream_copy_fuzzer zip_stream_copy_fuzzer.c)
target_link_libraries(zip_stream_copy_fuzzer PRIVATE ${PROJECT_NAME} $ENV{LIB_FUZZING_ENGINE})

add_executable(zip_create_fuzzer zip_create_fuzzer.c)
target_link_libraries(zip_create_fuzzer PRIVATE ${PROJECT_NAME} $ENV{LIB_FUZZING_ENGINE})

if (DEFINED ENV{OUT})
    install(TARGETS zip_stream_copy_fuzzer DESTINATION $ENV{OUT})
    install(TARGETS zip_create_fuzzer DESTINATION $ENV{OUT})
else ()
    message(WARNING "Cannot install if $OUT is not defined!")
endif ()
