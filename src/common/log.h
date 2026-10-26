#pragma once

#include <stdbool.h>

// Diagnostics channel for the core. The core formats messages and hands
// them to a sink; it never does IO itself. Whoever drives the machine (main,
// a bench tool, a unit test) installs the sink and decides where messages
// go. With no sink installed, logging is off and LOG() costs one branch.
//
// Global on purpose, so code deep in the call stack can log without a
// context being threaded through every function. Nothing the core computes
// may depend on it: it's write-only from the core's point of view.

enum LogLevel {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
};

// Receives one formatted message, without a trailing newline. ctx is the
// pointer given to log_set_sink, e.g. a FILE*.
typedef void (*LogSink)(void* ctx, enum LogLevel level, const char* msg);

// Messages longer than this are truncated.
#define LOG_MSG_LEN 256

// Install a sink receiving messages at min_level and above. A NULL sink
// turns logging off.
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
