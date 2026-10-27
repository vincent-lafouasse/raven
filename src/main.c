#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bus.h"
#include "debug.h"
#include "hart.h"
#include "loader.h"
#include "log.h"

#define TEMP_TOHOST 0x80001000

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

// riscv-tests' env/p reports its result by writing tohost once, then spins.
// The low word is enough: on RV32 the env writes it first, and the high word
// is always 0.
//   0             not finished yet
//   1             pass
//   odd, not 1    fail: test case (value >> 1) failed. An unexpected trap
//                 ORs in 1337, so the number is garbled; see the trap log
//   even          another HTIF command, which env/p never sends
static void temp_log_tohost(struct Bus bus[static 1])
{
    u32 val;
    const enum BusStatus status = bus_read(bus, TEMP_TOHOST, 4, &val);
    assert(status == BUS_OK);
    (void)status;  // unused when NDEBUG

    if (val == 0) {
        return;
    }

    if (val == 1) {
        LOG(LOG_LEVEL_INFO, "tohost 0x%x: pass", val);
        exit(EXIT_SUCCESS);
    }

    if (val & 1u) {
        LOG(LOG_LEVEL_WARN, "tohost 0x%x: fail, test case %u", val, val >> 1);
    } else {
        LOG(LOG_LEVEL_WARN, "tohost 0x%x: unsupported HTIF command", val);
    }
    exit(EXIT_FAILURE);
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
            // already taken: nothing was fetched, pc is at the handler
            continue;
        }

        // pc still points at the fetched instruction until it executes
        if (log_enabled(LOG_LEVEL_DEBUG)) {
            format_instruction(instr, FORMAT_PSEUDO, instruction_str);
            LOG(LOG_LEVEL_DEBUG, "0x%08x: %s", hart.pc, instruction_str);
        }

        // a trap here is already taken too, nothing to do
        hart_execute(&hart, bus, instr, &trap);

        temp_log_tohost(bus);
    }

    free(bus);
    return EXIT_SUCCESS;
}
