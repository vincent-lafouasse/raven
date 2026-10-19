#include "csr.h"

#include <stdbool.h>
#include <stddef.h>

#include "bit_utils.h"

#define MISA_RV32 (1 << 30)
#define MISA_I (1 << 8)

#define MISA_VALUE (MISA_RV32 | MISA_I)

static bool csr_can_access(u32 csr, enum PrivilegeMode mode)
{
    const u32 necessary_privilege_mode = u32_get_bits(csr, 8, 9);

    return (u32)mode >= necessary_privilege_mode;
}

enum CsrStatus csr_read(u32 csr, u32 out[static 1], enum PrivilegeMode mode)
{
    if (!csr_can_access(csr, mode)) {
        return CSR_INSUFFICIENT_PRIVILEGE;
    }

    switch (csr) {
        case CSR_MISA:
            *out = MISA_VALUE;
            return CSR_OK;
    }

    return CSR_NONEXISTENT;
}

enum CsrStatus csr_write(u32 csr, u32 value, enum PrivilegeMode mode)
{
    if (u32_get_bits(csr, 10, 11) == 3) {
        return CSR_READ_ONLY;
    }

    if (!csr_can_access(csr, mode)) {
        return CSR_INSUFFICIENT_PRIVILEGE;
    }

    (void)value;

    switch (csr) {
        case CSR_MISA:
            // ignore the write
            return CSR_OK;
    }

    return CSR_NONEXISTENT;
}

// Numbered CSR families, indexed by address - base.
static const char* const k_hpmcounter[29] = {
    "hpmcounter3",  "hpmcounter4",  "hpmcounter5",  "hpmcounter6",
    "hpmcounter7",  "hpmcounter8",  "hpmcounter9",  "hpmcounter10",
    "hpmcounter11", "hpmcounter12", "hpmcounter13", "hpmcounter14",
    "hpmcounter15", "hpmcounter16", "hpmcounter17", "hpmcounter18",
    "hpmcounter19", "hpmcounter20", "hpmcounter21", "hpmcounter22",
    "hpmcounter23", "hpmcounter24", "hpmcounter25", "hpmcounter26",
    "hpmcounter27", "hpmcounter28", "hpmcounter29", "hpmcounter30",
    "hpmcounter31"};
static const char* const k_hpmcounterh[29] = {
    "hpmcounter3h",  "hpmcounter4h",  "hpmcounter5h",  "hpmcounter6h",
    "hpmcounter7h",  "hpmcounter8h",  "hpmcounter9h",  "hpmcounter10h",
    "hpmcounter11h", "hpmcounter12h", "hpmcounter13h", "hpmcounter14h",
    "hpmcounter15h", "hpmcounter16h", "hpmcounter17h", "hpmcounter18h",
    "hpmcounter19h", "hpmcounter20h", "hpmcounter21h", "hpmcounter22h",
    "hpmcounter23h", "hpmcounter24h", "hpmcounter25h", "hpmcounter26h",
    "hpmcounter27h", "hpmcounter28h", "hpmcounter29h", "hpmcounter30h",
    "hpmcounter31h"};
static const char* const k_mhpmcounter[29] = {
    "mhpmcounter3",  "mhpmcounter4",  "mhpmcounter5",  "mhpmcounter6",
    "mhpmcounter7",  "mhpmcounter8",  "mhpmcounter9",  "mhpmcounter10",
    "mhpmcounter11", "mhpmcounter12", "mhpmcounter13", "mhpmcounter14",
    "mhpmcounter15", "mhpmcounter16", "mhpmcounter17", "mhpmcounter18",
    "mhpmcounter19", "mhpmcounter20", "mhpmcounter21", "mhpmcounter22",
    "mhpmcounter23", "mhpmcounter24", "mhpmcounter25", "mhpmcounter26",
    "mhpmcounter27", "mhpmcounter28", "mhpmcounter29", "mhpmcounter30",
    "mhpmcounter31"};
static const char* const k_mhpmcounterh[29] = {
    "mhpmcounter3h",  "mhpmcounter4h",  "mhpmcounter5h",  "mhpmcounter6h",
    "mhpmcounter7h",  "mhpmcounter8h",  "mhpmcounter9h",  "mhpmcounter10h",
    "mhpmcounter11h", "mhpmcounter12h", "mhpmcounter13h", "mhpmcounter14h",
    "mhpmcounter15h", "mhpmcounter16h", "mhpmcounter17h", "mhpmcounter18h",
    "mhpmcounter19h", "mhpmcounter20h", "mhpmcounter21h", "mhpmcounter22h",
    "mhpmcounter23h", "mhpmcounter24h", "mhpmcounter25h", "mhpmcounter26h",
    "mhpmcounter27h", "mhpmcounter28h", "mhpmcounter29h", "mhpmcounter30h",
    "mhpmcounter31h"};
static const char* const k_mhpmevent[29] = {
    "mhpmevent3",  "mhpmevent4",  "mhpmevent5",  "mhpmevent6",  "mhpmevent7",
    "mhpmevent8",  "mhpmevent9",  "mhpmevent10", "mhpmevent11", "mhpmevent12",
    "mhpmevent13", "mhpmevent14", "mhpmevent15", "mhpmevent16", "mhpmevent17",
    "mhpmevent18", "mhpmevent19", "mhpmevent20", "mhpmevent21", "mhpmevent22",
    "mhpmevent23", "mhpmevent24", "mhpmevent25", "mhpmevent26", "mhpmevent27",
    "mhpmevent28", "mhpmevent29", "mhpmevent30", "mhpmevent31"};
static const char* const k_pmpcfg[16] = {
    "pmpcfg0",  "pmpcfg1",  "pmpcfg2",  "pmpcfg3", "pmpcfg4",  "pmpcfg5",
    "pmpcfg6",  "pmpcfg7",  "pmpcfg8",  "pmpcfg9", "pmpcfg10", "pmpcfg11",
    "pmpcfg12", "pmpcfg13", "pmpcfg14", "pmpcfg15"};
static const char* const k_pmpaddr[64] = {
    "pmpaddr0",  "pmpaddr1",  "pmpaddr2",  "pmpaddr3",  "pmpaddr4",
    "pmpaddr5",  "pmpaddr6",  "pmpaddr7",  "pmpaddr8",  "pmpaddr9",
    "pmpaddr10", "pmpaddr11", "pmpaddr12", "pmpaddr13", "pmpaddr14",
    "pmpaddr15", "pmpaddr16", "pmpaddr17", "pmpaddr18", "pmpaddr19",
    "pmpaddr20", "pmpaddr21", "pmpaddr22", "pmpaddr23", "pmpaddr24",
    "pmpaddr25", "pmpaddr26", "pmpaddr27", "pmpaddr28", "pmpaddr29",
    "pmpaddr30", "pmpaddr31", "pmpaddr32", "pmpaddr33", "pmpaddr34",
    "pmpaddr35", "pmpaddr36", "pmpaddr37", "pmpaddr38", "pmpaddr39",
    "pmpaddr40", "pmpaddr41", "pmpaddr42", "pmpaddr43", "pmpaddr44",
    "pmpaddr45", "pmpaddr46", "pmpaddr47", "pmpaddr48", "pmpaddr49",
    "pmpaddr50", "pmpaddr51", "pmpaddr52", "pmpaddr53", "pmpaddr54",
    "pmpaddr55", "pmpaddr56", "pmpaddr57", "pmpaddr58", "pmpaddr59",
    "pmpaddr60", "pmpaddr61", "pmpaddr62", "pmpaddr63"};

// Non-architectural: names from the privileged spec's CSR listing, used for
// diagnostics and disassembly. Covers the standard CSRs of every privilege
// level and extension, not only the ones raven implements.
const char* csr_name(u32 csr)
{
    switch (csr) {
        case 0x001u:
            return "fflags";
        case 0x002u:
            return "frm";
        case 0x003u:
            return "fcsr";
        case 0xc00u:
            return "cycle";
        case 0xc01u:
            return "time";
        case 0xc02u:
            return "instret";
        case 0xc80u:
            return "cycleh";
        case 0xc81u:
            return "timeh";
        case 0xc82u:
            return "instreth";
        case 0x100u:
            return "sstatus";
        case 0x104u:
            return "sie";
        case 0x105u:
            return "stvec";
        case 0x106u:
            return "scounteren";
        case 0x10au:
            return "senvcfg";
        case 0x140u:
            return "sscratch";
        case 0x141u:
            return "sepc";
        case 0x142u:
            return "scause";
        case 0x143u:
            return "stval";
        case 0x144u:
            return "sip";
        case 0x14du:
            return "stimecmp";
        case 0x15du:
            return "stimecmph";
        case 0x180u:
            return "satp";
        case 0xf11u:
            return "mvendorid";
        case 0xf12u:
            return "marchid";
        case 0xf13u:
            return "mimpid";
        case 0xf14u:
            return "mhartid";
        case 0xf15u:
            return "mconfigptr";
        case 0x300u:
            return "mstatus";
        case 0x301u:
            return "misa";
        case 0x302u:
            return "medeleg";
        case 0x303u:
            return "mideleg";
        case 0x304u:
            return "mie";
        case 0x305u:
            return "mtvec";
        case 0x306u:
            return "mcounteren";
        case 0x310u:
            return "mstatush";
        case 0x312u:
            return "medelegh";
        case 0x340u:
            return "mscratch";
        case 0x341u:
            return "mepc";
        case 0x342u:
            return "mcause";
        case 0x343u:
            return "mtval";
        case 0x344u:
            return "mip";
        case 0x34au:
            return "mtinst";
        case 0x34bu:
            return "mtval2";
        case 0x30au:
            return "menvcfg";
        case 0x31au:
            return "menvcfgh";
        case 0x747u:
            return "mseccfg";
        case 0x757u:
            return "mseccfgh";
        case 0xb00u:
            return "mcycle";
        case 0xb02u:
            return "minstret";
        case 0xb80u:
            return "mcycleh";
        case 0xb82u:
            return "minstreth";
        case 0x320u:
            return "mcountinhibit";
        case 0x7a0u:
            return "tselect";
        case 0x7a1u:
            return "tdata1";
        case 0x7a2u:
            return "tdata2";
        case 0x7a3u:
            return "tdata3";
        case 0x7b0u:
            return "dcsr";
        case 0x7b1u:
            return "dpc";
        case 0x7b2u:
            return "dscratch0";
        case 0x7b3u:
            return "dscratch1";
        default:
            break;
    }

    if (csr >= 0xc03u && csr < 0xc03u + 29u) {
        return k_hpmcounter[csr - 0xc03u];
    }
    if (csr >= 0xc83u && csr < 0xc83u + 29u) {
        return k_hpmcounterh[csr - 0xc83u];
    }
    if (csr >= 0xb03u && csr < 0xb03u + 29u) {
        return k_mhpmcounter[csr - 0xb03u];
    }
    if (csr >= 0xb83u && csr < 0xb83u + 29u) {
        return k_mhpmcounterh[csr - 0xb83u];
    }
    if (csr >= 0x323u && csr < 0x323u + 29u) {
        return k_mhpmevent[csr - 0x323u];
    }
    if (csr >= 0x3a0u && csr < 0x3a0u + 16u) {
        return k_pmpcfg[csr - 0x3a0u];
    }
    if (csr >= 0x3b0u && csr < 0x3b0u + 64u) {
        return k_pmpaddr[csr - 0x3b0u];
    }

    return NULL;
}
