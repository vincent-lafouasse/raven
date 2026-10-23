#pragma once

#include <stdbool.h>

#include "privilege.h"
#include "trap.h"
#include "types.h"

struct Mstatus {
    bool mie, mpie;
    enum PrivilegeMode mpp;
};

struct Csr {
    u32 mie, mtvec, mstatush, mscratch, mcause, mtval, mip, mepc;
    struct Mstatus mstatus;
};

enum CsrStatus {
    CSR_OK,
    CSR_NONEXISTENT,
    CSR_READ_ONLY,
    CSR_INSUFFICIENT_PRIVILEGE,
};

struct Csr csr_new(void);

void csr_take_trap(struct Csr csr[static 1], struct Trap trap, u32 pc);

enum CsrStatus csr_read(u32 index,
                        u32 out[static 1],
                        enum PrivilegeMode mode,
                        struct Csr csr);

enum CsrStatus csr_write(u32 index,
                         u32 value,
                         enum PrivilegeMode mode,
                         struct Csr csr[static 1]);

const char* csr_name(u32 csr);
