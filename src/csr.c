#include "csr.h"

#include <stddef.h>

enum CsrStatus csr_read(const struct Hart hart[static 1],
                        u32 csr,
                        u32 out[static 1])
{
    (void)hart;
    (void)csr;
    (void)out;
    // TODO: implement the machine-mode CSRs
    return CSR_NONEXISTENT;
}

enum CsrStatus csr_write(struct Hart hart[static 1], u32 csr, u32 value)
{
    (void)hart;
    (void)csr;
    (void)value;
    // TODO: implement the machine-mode CSRs, CSR_READ_ONLY when bits 11:10
    // are 0b11
    return CSR_NONEXISTENT;
}

const char* csr_name(u32 csr)
{
    (void)csr;
    // TODO: name table
    return NULL;
}
