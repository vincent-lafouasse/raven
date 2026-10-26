#pragma once

#include <stdbool.h>

enum LogLevel {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
};

typedef void (*LogSink)(void* ctx, enum LogLevel level, const char* msg);

#define LOG_MSG_LEN 256

void log_set_sink(LogSink sink, void* ctx, enum LogLevel min_level);

bool log_enabled(enum LogLevel level);

// Prefer LOG(), which skips formatting (and evaluating the arguments) when
// the level is disabled.
void log_emit(enum LogLevel level, const char* fmt, ...)
    __attribute__((format(printf, 2, 3)));

#define LOG(level, ...)                   \
    do {                                  \
        if (log_enabled(level)) {         \
            log_emit(level, __VA_ARGS__); \
        }                                 \
    } while (0)
