#pragma once

#include "bus.h"
#include "csr.h"
#include "instruction.h"
#include "trap_cause.h"
#include "types.h"

enum TrapStatus {
    TRAP_NONE,
    TRAP_RAISED,
};

// Non-architectural: why raven raised the trap, beyond what cause and tval
// say. It never reaches a CSR or the guest, only whoever observes the trap
// (logs, the bench). Mostly useful for illegal instructions, where a single
// cause covers many unrelated reasons.
enum TrapDetailKind {
    DETAIL_NONE = 0,       // cause and tval say it all (ecall, ebreak, ...)
    DETAIL_DECODE,         // illegal instruction: the decoder rejected the word
    DETAIL_CSR,            // illegal instruction: a CSR access was refused
    DETAIL_UNIMPLEMENTED,  // illegal instruction: decoded but not executable
    DETAIL_BUS,            // fetch, load or store fault: what the bus reported
};

enum CsrAccess {
    CSR_ACCESS_READ,
    CSR_ACCESS_WRITE,
};

struct TrapDetail {
    enum TrapDetailKind kind;
    union {
        enum DecodeResult decode;  // DETAIL_DECODE
        struct {
            enum CsrStatus status;
            u16 index;
            enum CsrAccess access;
        } csr;                   // DETAIL_CSR
        enum Mnemonic mnemonic;  // DETAIL_UNIMPLEMENTED
        enum BusStatus bus;      // DETAIL_BUS
    } as;
};

struct Trap {
    enum TrapCause cause;  // architectural: goes to mcause
    u32 tval;              // architectural: goes to mtval
    struct TrapDetail detail;
};

// Constructors, one per detail kind, so `kind` always matches the member
// that was filled in.

static inline struct Trap make_trap(enum TrapCause cause, u32 tval)
{
    return (struct Trap){.cause = cause, .tval = tval};  // DETAIL_NONE
}

static inline struct Trap trap_decode(u32 raw, enum DecodeResult result)
{
    struct Trap t = make_trap(CAUSE_ILLEGAL_INSTRUCTION, raw);
    t.detail.kind = DETAIL_DECODE;
    t.detail.as.decode = result;
    return t;
}

static inline struct Trap trap_csr(u32 raw,
                                   enum CsrStatus status,
                                   u16 index,
                                   enum CsrAccess access)
{
    struct Trap t = make_trap(CAUSE_ILLEGAL_INSTRUCTION, raw);
    t.detail.kind = DETAIL_CSR;
    t.detail.as.csr.status = status;
    t.detail.as.csr.index = index;
    t.detail.as.csr.access = access;
    return t;
}

static inline struct Trap trap_unimplemented(u32 raw, enum Mnemonic mnemonic)
{
    struct Trap t = make_trap(CAUSE_ILLEGAL_INSTRUCTION, raw);
    t.detail.kind = DETAIL_UNIMPLEMENTED;
    t.detail.as.mnemonic = mnemonic;
    return t;
}

static inline struct Trap trap_bus(enum TrapCause cause,
                                   u32 addr,
                                   enum BusStatus status)
{
    struct Trap t = make_trap(cause, addr);
    t.detail.kind = DETAIL_BUS;
    t.detail.as.bus = status;
    return t;
}
