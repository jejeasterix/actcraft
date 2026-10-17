#pragma once

// Pic de mémoire résidente du processus, en octets (tests de bornes mémoire).
#include <QtGlobal>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
// windows.h doit précéder psapi.h : clang-format (SortIncludes) ne doit pas les inverser (même bloc que
// bench/src/memory.cpp).
// clang-format off
#include <windows.h>
#include <psapi.h>
// clang-format on
#else
#include <sys/resource.h>
#endif

inline qint64 peakMemoryBytes()
{
#ifdef Q_OS_WIN
    PROCESS_MEMORY_COUNTERS counters{};
    counters.cb = sizeof counters;
    return GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof counters)
               ? static_cast<qint64>(counters.PeakWorkingSetSize)
               : 0;
#else
    rusage usage{};
    getrusage(RUSAGE_SELF, &usage);
#ifdef Q_OS_MACOS
    return static_cast<qint64>(usage.ru_maxrss); // octets sous macOS
#else
    return static_cast<qint64>(usage.ru_maxrss) * 1024; // Kio ailleurs
#endif
#endif
}
