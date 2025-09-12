//
// Created by blomq on 2025-09-12.
//

#include "time_manager/utils/time_utils.h"

HighResTimeT GetHighResolutionTime(void) {
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
