# Utilized by OSSFuzz to build the harness(es) for continuous fuzz-testing
# OSSFuzz defines the following environment variables, that this target relies upon:
# CXX, CFLAGS, LIB_FUZZING_ENGINE, OUT

set(CMAKE_C_STANDARD 23)

add_definitions(-DNDEBUG)  # Do not want assertions

if (DEFINED ENV{CFLAGS})
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} $ENV{CFLAGS}")
endif ()

add_executable(fuzz_entry fuzz_entry.c)
target_link_libraries(fuzz_entry PRIVATE ${PROJECT_NAME} $ENV{LIB_FUZZING_ENGINE})

add_executable(fuzz_stream fuzz_stream.c)
target_link_libraries(fuzz_stream PRIVATE ${PROJECT_NAME} $ENV{LIB_FUZZING_ENGINE})

if (DEFINED ENV{OUT})
    install(TARGETS fuzz_entry DESTINATION $ENV{OUT})
    install(TARGETS fuzz_stream DESTINATION $ENV{OUT})
else ()
    message(WARNING "Cannot install if $OUT is not defined!")
endif ()