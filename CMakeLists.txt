cmake_minimum_required(VERSION 3.4...4.0)
project(time_manager C)

set(CMAKE_C_STANDARD 23)

option(TIME_MANAGER_BUILD_SHARED "Build time_manager as a shared library" ON)

set(TIMEMANAGER_SOURCES
        src/time_manager.c
        src/time_utils.c
)

set(TIMEMANAGER_HEADERS
        include/time_manager/time_manager.h
        include/time_manager/utils/time_utils.h
)

if(TIME_MANAGER_BUILD_SHARED)
    add_library(${PROJECT_NAME} SHARED)
else()
    add_library(${PROJECT_NAME} STATIC)
    # Consumers of the static lib need this define to disable dllimport/visibility
    target_compile_definitions(${PROJECT_NAME} PUBLIC TIME_MANAGER_STATIC)
endif()

add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# Make symbols hidden by default on GCC/Clang; export only what you mark
set_target_properties(${PROJECT_NAME} PROPERTIES
        C_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES
)

# Generate a portable export header with your existing macro name/path
include(GenerateExportHeader)
GENERATE_EXPORT_HEADER(${PROJECT_NAME}
        EXPORT_MACRO_NAME TIME_MANAGER_API
        BASE_NAME TIME_MANAGER
        EXPORT_FILE_NAME time_manager/time_manager_export.h
)

target_sources(${PROJECT_NAME} PRIVATE ${TIMEMANAGER_SOURCES})
target_sources(${PROJECT_NAME}
        PUBLIC
        FILE_SET HEADERS TYPE HEADERS
        BASE_DIRS include
        FILES ${TIMEMANAGER_HEADERS}
)

# Ensure both source include/ and generated header (in binary dir) are visible to consumers
target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<INSTALL_INTERFACE:include>
)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    include(GNUInstallDirs)

    # Install the library and its public headers (from FILE_SET)
    install(TARGETS ${PROJECT_NAME}
            EXPORT ${PROJECT_NAME}Targets
            FILE_SET HEADERS
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

    # Install the generated export header from the build tree
    install(FILES
            ${CMAKE_CURRENT_BINARY_DIR}/time_manager/time_manager_export.h
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/time_manager
    )

    # Export targets for find_package usage by consumers
    install(EXPORT ${PROJECT_NAME}Targets
            NAMESPACE ${PROJECT_NAME}::
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
    )
endif()

