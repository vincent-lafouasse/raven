#include "env.h"

// Linux RISC-V syscall numbers (asm-generic/unistd.h)
#define SYS_EXIT 93

static struct TrapAction action_exit(u8 exit_code)
{
    return (struct TrapAction){
        .kind = ACTION_EXIT,
        .exit_code = exit_code,
    };
}

static struct TrapAction action_fatal(enum FatalReason reason, u32 value)
{
    return (struct TrapAction){
        .kind = ACTION_FATAL,
        .reason = reason,
        .value = value,
    };
}

static struct TrapAction handle_ecall(struct Hart hart[static 1],
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
