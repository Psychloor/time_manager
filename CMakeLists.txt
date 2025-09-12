cmake_minimum_required(VERSION 3.4...4.0)

project(time_manager
        VERSION 1.0.0
        DESCRIPTION "A robust game timing and physics simulation library"
        LANGUAGES C
)

set(CMAKE_C_STANDARD_REQUIRED ON)

# Options
option(TIME_MANAGER_BUILD_SHARED "Build time_manager as a shared library" ON)

# Source files
set(TIMEMANAGER_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/time_manager.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/time_utils.c
)

set(TIMEMANAGER_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/include/time_manager/time_manager.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/time_manager/utils/time_utils.h
)

# Create library
if(TIME_MANAGER_BUILD_SHARED)
    add_library(${PROJECT_NAME} SHARED)
    message(STATUS "Building time_manager as SHARED library")
else()
    add_library(${PROJECT_NAME} STATIC)
    target_compile_definitions(${PROJECT_NAME} PUBLIC TIME_MANAGER_STATIC)
    message(STATUS "Building time_manager as STATIC library")
endif()

# Create alias for modern CMake usage
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# Set target properties
set_target_properties(${PROJECT_NAME} PROPERTIES
        C_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        POSITION_INDEPENDENT_CODE ON
)

# Generate export header
include(GenerateExportHeader)
GENERATE_EXPORT_HEADER(${PROJECT_NAME}
        EXPORT_MACRO_NAME TIME_MANAGER_API
        BASE_NAME TIME_MANAGER
        EXPORT_FILE_NAME time_manager/time_manager_export.h
)

# Add sources
target_sources(${PROJECT_NAME} PRIVATE ${TIMEMANAGER_SOURCES})
target_sources(${PROJECT_NAME}
        PUBLIC
        FILE_SET HEADERS TYPE HEADERS
        BASE_DIRS include
        FILES ${TIMEMANAGER_HEADERS}
)

# Include directories
target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(${PROJECT_NAME} PUBLIC c_std_11)

# Platform-specific linking
if(UNIX AND NOT APPLE)
    find_library(MATH_LIBRARY m)
    if(MATH_LIBRARY)
        target_link_libraries(${PROJECT_NAME} PRIVATE ${MATH_LIBRARY})
    endif()

    # Check if we need to link against rt for clock_gettime
    include(CheckLibraryExists)
    check_library_exists(rt clock_gettime "" HAVE_CLOCK_GETTIME_IN_RT)
    if(HAVE_CLOCK_GETTIME_IN_RT)
        target_link_libraries(${PROJECT_NAME} PRIVATE rt)
    endif()
endif()

# Compiler-specific flags
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(${PROJECT_NAME} PRIVATE -g -O0)
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE -O3)
    endif()
endif()

# Installation (only if this is the main project)
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    # Install the library
    install(TARGETS ${PROJECT_NAME}
            EXPORT ${PROJECT_NAME}Targets
            FILE_SET HEADERS
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

    # Install generated export header
    install(FILES
            ${CMAKE_CURRENT_BINARY_DIR}/time_manager/time_manager_export.h
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/time_manager
    )

    # Export targets
    install(EXPORT ${PROJECT_NAME}Targets
            NAMESPACE ${PROJECT_NAME}::
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
    )

    # Create and install package config file
    configure_package_config_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/cmake/time_managerConfig.cmake.in
            ${CMAKE_CURRENT_BINARY_DIR}/time_managerConfig.cmake
            INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
    )

    # Create and install package version file
    write_basic_package_version_file(
            ${CMAKE_CURRENT_BINARY_DIR}/time_managerConfigVersion.cmake
            VERSION ${PROJECT_VERSION}
            COMPATIBILITY SameMajorVersion
    )

    install(FILES
            ${CMAKE_CURRENT_BINARY_DIR}/time_managerConfig.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/time_managerConfigVersion.cmake
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
    )

    # pkg-config support (optional but nice to have)
    configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/cmake/time_manager.pc.in
            ${CMAKE_CURRENT_BINARY_DIR}/time_manager.pc
            @ONLY
    )

    install(FILES
            ${CMAKE_CURRENT_BINARY_DIR}/time_manager.pc
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
    )

    # Print summary
    message(STATUS "")
    message(STATUS "=== Time Manager Configuration ===")
    message(STATUS "Version: ${PROJECT_VERSION}")
    message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
    message(STATUS "Library type: ${TIME_MANAGER_BUILD_SHARED}")
    message(STATUS "Build examples: ${TIME_MANAGER_BUILD_EXAMPLES}")
    message(STATUS "Build tests: ${TIME_MANAGER_BUILD_TESTS}")
    message(STATUS "==================================")
endif()