#pragma once

#include "trap_cause.h"
#include "types.h"

enum TrapStatus {
    TRAP_NONE,
    TRAP_RAISED,
};

struct Trap {
    enum TrapCause cause;
    u32 tval;
};

static inline struct Trap make_trap(enum TrapCause cause, u32 tval)
{
    return (struct Trap){.cause = cause, .tval = tval};
}
