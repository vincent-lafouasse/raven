#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bus.h"
#include "hart.h"
#include "loader.h"
#include "utils.h"

#define USAGE                           \
    "usage: raven --flat program.bin\n" \
    "       raven --elf program.elf"

// Linux RISC-V syscall numbers (asm-generic/unistd.h)
#define SYS_EXIT 93

__attribute__((noreturn)) static void die(const char* fmt, ...)
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

enum TrapActionKind {
    ACTION_CONTINUE,
    ACTION_EXIT,
    ACTION_FATAL,
};

enum FatalReason {
    FATAL_UNKNOWN_SYSCALL,  // value: a7
    FATAL_BAD_EXIT_VALUE,   // value: a0
    FATAL_FAILED_DECODE,    // value: instruction bits
    FATAL_TRAP,             // value: trap cause
};

struct TrapAction {
    enum TrapActionKind kind;
    u8 exit_code;             // EXIT
    enum FatalReason reason;  // FATAL
    u32 value;                // FATAL
};

struct TrapAction action_exit(u8 exit_code)
{
    return (struct TrapAction){
        .kind = ACTION_EXIT,
        .exit_code = exit_code,
    };
}

struct TrapAction action_fatal(enum FatalReason reason, u32 value)
{
    return (struct TrapAction){
        .kind = ACTION_FATAL,
        .reason = reason,
        .value = value,
    };
}

struct TrapAction handle_ecall(struct Hart hart[static 1],
                               struct Bus bus[static 1])
{
    (void)bus;

    switch (hart->x[REG_A7]) {
        case SYS_EXIT:
            // the OS only keeps 8 bits: 256 would read as 0
            if (hart->x[REG_A0] > 0xff) {
                return action_fatal(FATAL_BAD_EXIT_VALUE, hart->x[REG_A0]);
            } else {
                return action_exit(hart->x[REG_A0]);
            }
        default:
            return action_fatal(FATAL_UNKNOWN_SYSCALL, hart->x[REG_A7]);
    }
}

struct TrapAction handle_trap(struct Trap trap,
                              struct Hart hart[static 1],
                              struct Bus bus[static 1])
{
    switch (trap.cause) {
        case CAUSE_ECALL_M:
            return handle_ecall(hart, bus);
        case CAUSE_ILLEGAL_INSTRUCTION: {
            // tval holds the instruction bits. Words that do not decode get
            // their own reason, so the report can say why.
            struct Instruction unused;
            if (decode_instruction(trap.tval, &unused) != DR_OK) {
                return action_fatal(FATAL_FAILED_DECODE, trap.tval);
            }
            return action_fatal(FATAL_TRAP, (u32)trap.cause);
        }
        default:
            return action_fatal(FATAL_TRAP, (u32)trap.cause);
    }
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

int main(int ac, char** av)
{
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

    char hart_str[HART_DUMP_LEN] = {0};
    hart_dump(&hart, hart_str);
    printf("%s\n", hart_str);

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
        printf("%s\n", instruction_str);

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

        hart_dump(&hart, hart_str);
        printf("%s\n", hart_str);
    }

    free(bus);
    return EXIT_SUCCESS;
}
