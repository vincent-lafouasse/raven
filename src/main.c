#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bus.h"
#include "debug.h"
#include "env.h"
#include "hart.h"
#include "loader.h"
#include "log.h"

#define TEMP_TOHOST  0x80001000

#define USAGE                           \
    "usage: raven --flat program.bin\n" \
    "       raven --elf program.elf"

__attribute__((noreturn, format(printf, 1, 2))) static void die(const char* fmt,
                                                                ...)
{
    fflush(stdout);

    va_list ap;
    va_start(ap, fmt);
    fputs("raven: ", stderr);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);
    exit(EXIT_FAILURE);
}

static const char* load_status_str(enum LoadStatus s)
{
    switch (s) {
        case LOAD_OK:
            return "ok";
        case LOAD_CANNOT_OPEN:
            return "cannot open file";
        case LOAD_CANNOT_READ:
            return "cannot read file";
        case LOAD_TOO_BIG:
            return "program does not fit in RAM";
        case LOAD_UNSUPPORTED:
            return "unsupported format";
    }
    return "unknown error";
}

static const char* decode_result_str(enum DecodeResult r)
{
    switch (r) {
        case DR_OK:
            return "ok";
        case DR_BAD_LENGTH:
            return "not a 32-bit instruction";
        case DR_BAD_OPCODE:
            return "unknown opcode";
        case DR_BAD_FUNCT3:
            return "bad funct3";
        case DR_BAD_FUNCT7:
            return "bad funct7";
        case DR_UNSUPPORTED:
            return "valid but unsupported";
    }
    return "unknown decode error";
}

__attribute__((noreturn)) static void report_fatal_and_die(
    struct TrapAction action,
    struct Trap trap,
    const struct Hart hart[static 1])
{
    switch (action.reason) {
        case FATAL_UNKNOWN_SYSCALL:
            die("unknown syscall a7=0x%08x at pc=0x%08x", action.value,
                hart->pc);
        case FATAL_BAD_EXIT_VALUE:
            die("exit code %u does not fit in 8 bits (pc=0x%08x)", action.value,
                hart->pc);
        case FATAL_FAILED_DECODE: {
            struct Instruction unused;
            const enum DecodeResult r =
                decode_instruction(action.value, &unused);
            die("failed to decode 0x%08x at pc=0x%08x: %s", action.value,
                hart->pc, decode_result_str(r));
        }
        case FATAL_TRAP:
            die("%s at pc=0x%08x (tval=0x%08x)", trap_cause_str(trap.cause),
                hart->pc, trap.tval);
    }
    UNREACHABLE();
}

static const char* log_level_str(enum LogLevel level)
{
    switch (level) {
        case LOG_LEVEL_DEBUG:
            return "debug";
        case LOG_LEVEL_INFO:
            return "info";
        case LOG_LEVEL_WARN:
            return "warn";
    }
    return "?";
}

static void stderr_sink(void* ctx, enum LogLevel level, const char* msg)
{
    fflush(stdout);
    fprintf((FILE*)ctx, "[%s] %s\n", log_level_str(level), msg);
}

int main(int ac, char** av)
{
    log_set_sink(stderr_sink, stderr, LOG_LEVEL_DEBUG);

    if (ac != 3) {
        die(USAGE);
    }

    const char* const flag = av[1];
    const char* const binary_path = av[2];

    enum LoadStatus (*load)(struct Bus*, const char*, u32*);
    if (strcmp(flag, "--flat") == 0) {
        load = load_flat;
    } else if (strcmp(flag, "--elf") == 0) {
        load = load_elf;
    } else {
        die("unknown option %s\n" USAGE, flag);
    }

    struct Bus* bus = calloc(1, sizeof(*bus));
    if (!bus) {
        die("out of memory");
    }

    u32 entry;
    const enum LoadStatus status = load(bus, binary_path, &entry);
    if (status != LOAD_OK) {
        die("%s: %s", binary_path, load_status_str(status));
    }

    struct Hart hart;
    hart_reset(&hart, entry);

    char instruction_str[INSTRUCTION_STR_LEN] = {0};

    while (1) {
        struct Trap trap;
        struct Instruction instr;

        if (hart_fetch(&hart, bus, &instr, &trap) == TRAP_RAISED) {
            const struct TrapAction action = handle_trap(trap, &hart, bus);
            switch (action.kind) {
                case ACTION_CONTINUE:
                    // no instruction was fetched, start the next tick
                    continue;
                case ACTION_EXIT:
                    exit(action.exit_code);
                case ACTION_FATAL:
                    report_fatal_and_die(action, trap, &hart);
            }
        }

        format_instruction(instr, instruction_str);
        LOG(LOG_LEVEL_DEBUG, "%s", instruction_str);

        if (hart_execute(&hart, bus, instr, &trap) == TRAP_RAISED) {
            const struct TrapAction action = handle_trap(trap, &hart, bus);
            switch (action.kind) {
                case ACTION_CONTINUE:
                    break;
                case ACTION_EXIT:
                    exit(action.exit_code);
                case ACTION_FATAL:
                    report_fatal_and_die(action, trap, &hart);
            }
        }
    }

    free(bus);
    return EXIT_SUCCESS;
}
