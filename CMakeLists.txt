cmake_minimum_required(VERSION 4.0)
project(time_manager C)

set(CMAKE_C_STANDARD 23)

set(TIMEMANAGER_SOURCES
        src/time_manager.c
)

set(TIMEMANAGER_HEADERS
        include/time_manager/time_manager.h
        include/time_manager/utils/time_utils.h
        include/time_manager/time_manager_export.h
)

add_library(${PROJECT_NAME} SHARED)

target_sources(${PROJECT_NAME} PRIVATE ${TIMEMANAGER_SOURCES})
target_sources(${PROJECT_NAME}
        PUBLIC
        FILE_SET HEADERS TYPE HEADERS
        BASE_DIRS include
        FILES ${TIMEMANAGER_HEADERS}
)