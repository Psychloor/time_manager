//
// Created by blomq on 2025-09-12.
//

#ifndef TIME_MANAGER_TIME_MANAGER_EXPORT_H
#define TIME_MANAGER_TIME_MANAGER_EXPORT_H

#ifdef _WIN32
#define TIME_MANAGER_API __declspec(dllexport)
#else
#define TIME_MANAGER_API
#endif

#endif //TIME_MANAGER_TIME_MANAGER_EXPORT_H