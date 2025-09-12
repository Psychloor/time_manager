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

TIME_MANAGER_API HighResTimeT GetHighResolutionTime(void);

#ifdef __cplusplus
}
#endif

#endif //TIME_MANAGER_TIME_UTILS_H
