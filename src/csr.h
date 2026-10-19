#pragma once

#include "privilege.h"
#include "types.h"

#define CSR_MSTATUS 0x300u
#define CSR_MISA 0x301u
#define CSR_MIE 0x304u
#define CSR_MTVEC 0x305u
#define CSR_MSTATUSH 0x310u
#define CSR_MSCRATCH 0x340u
#define CSR_MEPC 0x341u
#define CSR_MCAUSE 0x342u
#define CSR_MTVAL 0x343u
#define CSR_MIP 0x344u

#define CSR_MCYCLE 0xb00u
#define CSR_MINSTRET 0xb02u
#define CSR_MCYCLEH 0xb80u
#define CSR_MINSTRETH 0xb82u

// user-level read-only aliases of the counters (Zicntr)
#define CSR_CYCLE 0xc00u
#define CSR_TIME 0xc01u
#define CSR_INSTRET 0xc02u
#define CSR_CYCLEH 0xc80u
#define CSR_TIMEH 0xc81u
#define CSR_INSTRETH 0xc82u

#define CSR_MVENDORID 0xf11u
#define CSR_MARCHID 0xf12u
#define CSR_MIMPID 0xf13u
#define CSR_MHARTID 0xf14u

struct Csr {
    u32 mstatus, misa, mie, mtvec, mstatush, mscratch, mcause, mtval, mip;
};

enum CsrStatus {
    CSR_OK,
    CSR_NONEXISTENT,
    CSR_READ_ONLY,
};

enum CsrStatus csr_read(u32 csr, u32 out[static 1], enum PrivilegeMode mode);

enum CsrStatus csr_write(u32 csr, u32 value, enum PrivilegeMode mode);

const char* csr_name(u32 csr);
