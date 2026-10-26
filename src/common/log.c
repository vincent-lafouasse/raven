#include "log.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

static LogSink g_sink = NULL;
static void* g_ctx = NULL;
static enum LogLevel g_min_level = LOG_LEVEL_WARN;

void log_set_sink(LogSink sink, void* ctx, enum LogLevel min_level)
{
    g_sink = sink;
    g_ctx = ctx;
    g_min_level = min_level;
}

bool log_enabled(enum LogLevel level)
{
    return g_sink != NULL && level >= g_min_level;
}

void log_emit(enum LogLevel level, const char* fmt, ...)
{
    if (!log_enabled(level)) {
        return;
    }

    // formatting only: the sink decides whether this ever reaches IO
    char msg[LOG_MSG_LEN];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, sizeof msg, fmt, args);
    va_end(args);

    g_sink(g_ctx, level, msg);
}
