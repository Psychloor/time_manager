cmake_minimum_required(VERSION 4.0)
project(time_manager C)

set(CMAKE_C_STANDARD 23)

set(TIMEMANAGER_SOURCES
        src/time_manager.c
)

set(TIMEMANAGER_HEADERS
        include/time_manager/time_manager.h
        include/time_manager/utils/time_utils.h
)

add_library(${PROJECT_NAME} SHARED)

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
