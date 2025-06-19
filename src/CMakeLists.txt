# zip
set(Header_Files "miniz.h" "zip.h")
source_group("Header Files" FILES "${Header_Files}")

set(Source_Files "zip.c")
source_group("Source Files" FILES "${Source_Files}")

add_library(${PROJECT_NAME} "${Header_Files}" "${Source_Files}")
set_target_properties("${PROJECT_NAME}" PROPERTIES LINKER_LANGUAGE C)

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

include(GNUInstallDirs)
target_include_directories(${PROJECT_NAME} PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}>"
)

set_property(TARGET "${PROJECT_NAME}" PROPERTY VERSION "${${PROJECT_NAME}_VERSION}")
set_property(TARGET "${PROJECT_NAME}" PROPERTY SOVERSION "${${PROJECT_NAME}_VERSION_MAJOR}")

install(FILES ${Header_Files} DESTINATION "${INSTALL_PATH}")
