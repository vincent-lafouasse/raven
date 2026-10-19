#pragma once

#if defined(__GNUC__) || defined(__clang__)
#define UNREACHABLE()            \
    do {                         \
        __builtin_trap();        \
        __builtin_unreachable(); \
    } while (0)
#elif defined(_MSC_VER)
#include <intrin.h>  // for __debugbreak()
#define UNREACHABLE()   \
    do {                \
        __debugbreak(); \
        __assume(0);    \
    } while (0)
#else
#define UNREACHABLE() abort()
#endif

#ifndef TRACE_TRAP
#if defined(__GNUC__) || defined(__clang__)
#define TRACE_TRAP() __builtin_trap()
#elif defined(_MSC_VER)
#define TRACE_TRAP() __debugbreak()
#else
#define TRACE_TRAP() abort()
#endif
#endif
