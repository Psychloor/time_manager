//
// Created by blomq on 2025-09-12.
//

#ifndef TIME_MANAGER_TIME_UTILS_H
#define TIME_MANAGER_TIME_UTILS_H

#include <time.h>

#include "time_manager/time_manager_export.h"

#ifdef _WIN32
#include <windows.h>
#endif

// @formatter:off
#ifdef __cplusplus
extern "C" {
#endif
// @formatter:on

typedef struct
{
    long long nanoseconds;
} HighResTimeT;

TIME_MANAGER_API inline HighResTimeT GetTime(void)
{
    HighResTimeT result;

    #ifdef _WIN32
    LARGE_INTEGER freq, counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    result.nanoseconds = (counter.QuadPart * 1000000000LL) / freq.QuadPart;
    #else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    result.nanoseconds = ts.tv_sec * 1000000000LL + ts.tv_nsec;
    #endif

    return result;
}

#ifdef __cplusplus
}
#endif

#endif //TIME_MANAGER_TIME_UTILS_H
