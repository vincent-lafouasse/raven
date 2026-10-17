#pragma once

// raven's execution environment: the policy deciding what happens to a trap
// the hart raises (syscalls, exit, fatal errors). The hart only raises traps;
// main performs the resulting IO.

#include "bus.h"
#include "hart.h"
#include "trap.h"
#include "types.h"

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

// Decides what to do with a trap raised by the hart. Does no IO.
struct TrapAction handle_trap(struct Trap trap,
                              struct Hart hart[static 1],
                              struct Bus bus[static 1]);
