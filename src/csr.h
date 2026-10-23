#pragma once

#include <stdbool.h>

#include "privilege.h"
#include "types.h"

struct Mstatus {
    bool mie, mpie;
    enum PrivilegeMode mpp;
};

struct Csr {
    u32 mie, mtvec, mstatush, mscratch, mcause, mtval, mip;
    struct Mstatus mstatus;
};

enum CsrStatus {
    CSR_OK,
    CSR_NONEXISTENT,
    CSR_READ_ONLY,
    CSR_INSUFFICIENT_PRIVILEGE,
};

struct Csr csr_new(void);

enum CsrStatus csr_read(u32 csr,
                        u32 out[static 1],
                        enum PrivilegeMode mode,
                        struct Csr registers);

enum CsrStatus csr_write(u32 csr,
                         u32 value,
                         enum PrivilegeMode mode,
                         struct Csr registers[static 1]);

const char* csr_name(u32 csr);
