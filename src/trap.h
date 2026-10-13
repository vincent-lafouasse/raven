#pragma once

#include "types.h"

// Traps, shaped like the RISC-V privileged spec so that raven can later take
// them architecturally (mcause/mtval/mepc, jump to mtvec) without changing
// the code that raises them.
//
// For now a trap is reported by the execution environment (main.c): ecall is
// a syscall, everything else is fatal.

// Exception codes, equal to the values the hardware writes to mcause.
// Interrupts share the same register with the top bit set (see below).
enum TrapCause {
    CAUSE_MISALIGNED_FETCH = 0,     // tval: target address
    CAUSE_FETCH_ACCESS = 1,         // tval: fetch address
    CAUSE_ILLEGAL_INSTRUCTION = 2,  // tval: instruction bits
    CAUSE_BREAKPOINT = 3,           // tval: pc of the ebreak
    CAUSE_MISALIGNED_LOAD = 4,      // tval: data address
    CAUSE_LOAD_ACCESS = 5,          // tval: data address
    CAUSE_MISALIGNED_STORE = 6,     // tval: data address
    CAUSE_STORE_ACCESS = 7,         // tval: data address
    CAUSE_ECALL_U = 8,              // tval: 0
    CAUSE_ECALL_S = 9,              // tval: 0
    CAUSE_ECALL_M = 11,             // tval: 0
    CAUSE_FETCH_PAGE_FAULT = 12,    // tval: virtual address (Sv32, later)
    CAUSE_LOAD_PAGE_FAULT = 13,
    CAUSE_STORE_PAGE_FAULT = 15,
};

// Interrupt codes, written to mcause as (CAUSE_INTERRUPT | code). Unused until
// raven has a timer and interrupt controllers.
#define CAUSE_INTERRUPT 0x80000000u

enum InterruptCode {
    IRQ_S_SOFTWARE = 1,
    IRQ_M_SOFTWARE = 3,
    IRQ_S_TIMER = 5,
    IRQ_M_TIMER = 7,
    IRQ_S_EXTERNAL = 9,
    IRQ_M_EXTERNAL = 11,
};

struct Trap {
    enum TrapCause cause;
    u32 tval;  // what the hardware would write to mtval, see the enum
};

static inline struct Trap make_trap(enum TrapCause cause, u32 tval)
{
    return (struct Trap){.cause = cause, .tval = tval};
}

static inline const char* trap_cause_str(enum TrapCause cause)
{
    switch (cause) {
        case CAUSE_MISALIGNED_FETCH:
            return "instruction address misaligned";
        case CAUSE_FETCH_ACCESS:
            return "instruction access fault";
        case CAUSE_ILLEGAL_INSTRUCTION:
            return "illegal instruction";
        case CAUSE_BREAKPOINT:
            return "breakpoint";
        case CAUSE_MISALIGNED_LOAD:
            return "load address misaligned";
        case CAUSE_LOAD_ACCESS:
            return "load access fault";
        case CAUSE_MISALIGNED_STORE:
            return "store address misaligned";
        case CAUSE_STORE_ACCESS:
            return "store access fault";
        case CAUSE_ECALL_U:
            return "environment call from U-mode";
        case CAUSE_ECALL_S:
            return "environment call from S-mode";
        case CAUSE_ECALL_M:
            return "environment call from M-mode";
        case CAUSE_FETCH_PAGE_FAULT:
            return "instruction page fault";
        case CAUSE_LOAD_PAGE_FAULT:
            return "load page fault";
        case CAUSE_STORE_PAGE_FAULT:
            return "store page fault";
    }
    return "unknown trap";
}
