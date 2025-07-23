cmake_minimum_required(VERSION 3.14)

project(zip
        LANGUAGES C
        VERSION "0.3.0")
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
set(CMAKE_VERBOSE_MAKEFILE ON)

# Enable building tests only if the project is being built as a standalone one
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    option(CMAKE_DISABLE_TESTING "Disable test creation" OFF)
else ()
    option(CMAKE_DISABLE_TESTING "Disable test creation" ON)
endif ()

option(CMAKE_ENABLE_SANITIZERS "Enable zip sanitizers" OFF)
option(ZIP_STATIC_PIC "Build static zip with PIC" ON)
option(ZIP_BUILD_DOCS "Generate API documentation with Doxygen" OFF)
option(ZIP_BUILD_FUZZ "Build fuzz targets" OFF)

if(ZIP_ENABLE_SHARABLE_FILE_OPEN)
    add_definitions(-DZIP_ENABLE_SHARABLE_FILE_OPEN)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    # large file support
    add_definitions(-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64)
endif()

# zip
set(Header_Files "src/miniz.h" "src/zip.h")
source_group("Header Files" FILES "${Header_Files}")

set(Source_Files "src/zip.c")
source_group("Source Files" FILES "${Source_Files}")

add_library(${PROJECT_NAME} "${Header_Files}" "${Source_Files}")
set_target_properties("${PROJECT_NAME}" PROPERTIES LINKER_LANGUAGE C)

set(gcc_like "$<COMPILE_LANG_AND_ID:C,CXX,ARMClang,AppleClang,Clang,GNU,LCC>")
set(msvc "$<COMPILE_LANG_AND_ID:C,CXX,MSVC>")
target_compile_options(
        "${PROJECT_NAME}"
        INTERFACE
        "$<${gcc_like}:$<BUILD_INTERFACE:-Wall;-Wextra;-Wshadow;-Wformat=2;-Wunused;-Wno-long-long;-pedantic>>"
        "$<${msvc}:$<BUILD_INTERFACE:-W3;-WX;-Zi;-permissive->>"
)

add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

if(ZIP_STATIC_PIC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE 1)
endif()

set_property(TARGET ${PROJECT_NAME} PROPERTY C_VISIBILITY_PRESET hidden)
if(BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME}
            PUBLIC ZIP_SHARED
            PRIVATE ZIP_BUILD_SHARED
    )
endif()

target_include_directories(${PROJECT_NAME} PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

# test
if (NOT CMAKE_DISABLE_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()
if (CMAKE_ENABLE_SANITIZERS)
    find_package(Sanitizers)
    add_sanitizers(${PROJECT_NAME})
endif()

set(CMAKE_C_STANDARD 90)

####

# fuzz
if (ZIP_BUILD_FUZZ)
    if (NOT DEFINED ENV{LIB_FUZZING_ENGINE})
        message(FATAL_ERROR "LIB_FUZZING_ENGINE is not defined")
    endif()
    add_subdirectory(fuzz)
endif()
###

set(CONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
set(CMAKE_INSTALL_INCLUDEDIR "include")

set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")

# Configuration
set(VERSION_CONFIG "${GENERATED_DIR}/${PROJECT_NAME}ConfigVersion.cmake")
set(PROJECT_CONFIG "${GENERATED_DIR}/${PROJECT_NAME}Config.cmake")
set(TARGETS_EXPORT_NAME "${PROJECT_NAME}Targets")
set(NAMESPACE "${PROJECT_NAME}::")

# Include module with function 'write_basic_package_version_file'
include(CMakePackageConfigHelpers)

# Note: PROJECT_VERSION is used as a VERSION
write_basic_package_version_file(
        "${VERSION_CONFIG}" COMPATIBILITY SameMajorVersion
)

# Use variables:
#   * TARGETS_EXPORT_NAME
#   * PROJECT_NAME
configure_package_config_file(
        "cmake/Config.cmake.in"
        "${PROJECT_CONFIG}"
        INSTALL_DESTINATION "${CONFIG_INSTALL_DIR}"
)

install(
        FILES "${PROJECT_CONFIG}" "${VERSION_CONFIG}"
        DESTINATION "${CONFIG_INSTALL_DIR}"
)

install(
        EXPORT "${TARGETS_EXPORT_NAME}"
        NAMESPACE "${NAMESPACE}"
        DESTINATION "${CONFIG_INSTALL_DIR}"
)

install(TARGETS ${PROJECT_NAME}
        EXPORT ${TARGETS_EXPORT_NAME}
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(FILES ${PROJECT_SOURCE_DIR}/src/zip.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/zip)

# uninstall target (https://gitlab.kitware.com/cmake/community/wikis/FAQ#can-i-do-make-uninstall-with-cmake)
if(NOT TARGET uninstall)
    configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
            "${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake"
            IMMEDIATE @ONLY)

    add_custom_target(uninstall
            COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake)
endif()

if(ZIP_BUILD_DOCS)
    find_package(Doxygen REQUIRED)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
    add_custom_target(doc
            ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen" VERBATIM)
endif()
