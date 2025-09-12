cmake_minimum_required(VERSION 3.16)

add_executable(time_manager_tests
        test_time_manager.c
)

# Public headers + generated export header dir
target_include_directories(time_manager_tests PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_BINARY_DIR}
)

# Link the library target defined in the root CMakeLists.txt
target_link_libraries(time_manager_tests PRIVATE time_manager)

# Linux needs libm for fabs(), etc.
if (UNIX AND NOT APPLE)
    target_link_libraries(time_manager_tests PRIVATE m)
endif()

# Be strict in tests
if(MSVC)
    target_compile_options(time_manager_tests PRIVATE /W4 /WX)
else()
    target_compile_options(time_manager_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

# Use the absolute path for CTest (multi-config safe)
add_test(NAME time_manager_tests COMMAND $<TARGET_FILE:time_manager_tests>)

# Make sure the test can find the shared lib at runtime
if (WIN32)
    # Copy DLL next to the test exe
    add_custom_command(TARGET time_manager_tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:time_manager>
            $<TARGET_FILE_DIR:time_manager_tests>)
    # Also set PATH for safety when CTest launches the exe
    set_tests_properties(time_manager_tests PROPERTIES
            ENVIRONMENT "PATH=$<TARGET_FILE_DIR:time_manager_tests>;$ENV{PATH}")
elseif(APPLE)
    # Tell dyld where to look
    set_tests_properties(time_manager_tests PROPERTIES
            ENVIRONMENT "DYLD_LIBRARY_PATH=$<TARGET_FILE_DIR:time_manager>:$ENV{DYLD_LIBRARY_PATH}")
elseif(UNIX)
    # Tell the dynamic linker where to look
    set_tests_properties(time_manager_tests PROPERTIES
            ENVIRONMENT "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:time_manager>:$ENV{LD_LIBRARY_PATH}")
endif()