#include "format_trap.h"

#include <stdarg.h>
#include <stdio.h>

static const char* decode_str(enum DecodeResult r)
{
    switch (r) {
        case DR_OK:
            return "ok";
        case DR_BAD_LENGTH:
            return "not a 32-bit instruction";
        case DR_BAD_OPCODE:
            return "unknown opcode";
        case DR_BAD_FUNCT3:
            return "bad funct3";
        case DR_BAD_FUNCT7:
            return "bad funct7";
        case DR_UNSUPPORTED:
            return "valid but unsupported";
    }
    return "unknown decode error";
}

static const char* csr_status_str(enum CsrStatus s)
{
    switch (s) {
        case CSR_OK:
            return "ok";
        case CSR_NONEXISTENT:
            return "nonexistent";
        case CSR_READ_ONLY:
            return "read-only";
        case CSR_INSUFFICIENT_PRIVILEGE:
            return "insufficient privilege";
    }
    return "unknown CSR status";
}

static const char* bus_status_str(enum BusStatus s)
{
    switch (s) {
        case BUS_OK:
            return "ok";
        case BUS_UNMAPPED:
            return "unmapped address";
        case BUS_MISALIGNED:
            return "misaligned access";
    }
    return "unknown bus status";
}

static const char* mode_str(enum PrivilegeMode m)
{
    switch (m) {
        case MODE_USER:
            return "U";
        case MODE_SUPERVISOR:
            return "S";
        case MODE_MACHINE:
            return "M";
    }
    return "?";
}

// appends to out, never past TRAP_STR_LEN; *n stays a valid length
__attribute__((format(printf, 3, 4))) static void
append(char out[static TRAP_STR_LEN], size_t n[static 1], const char* fmt, ...)
{
    if (*n >= TRAP_STR_LEN - 1) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    const int written = vsnprintf(out + *n, TRAP_STR_LEN - *n, fmt, args);
    va_end(args);

    if (written > 0) {
        *n += (size_t)written;
        if (*n > TRAP_STR_LEN - 1) {
            *n = TRAP_STR_LEN - 1;  // truncated
        }
    }
}

void format_trap(struct Trap trap, char out[static TRAP_STR_LEN])
{
    size_t n = 0;
    out[0] = '\0';

    append(out, &n, "%s (cause %u, tval 0x%08x)", trap_cause_str(trap.cause),
           (unsigned)trap.cause, trap.tval);

    // tval holds the instruction for the illegal instruction cases: show
    // what it was, when it decodes
    struct Instruction instr;
    char instr_str[INSTRUCTION_STR_LEN];
    const bool has_instr = trap.cause == CAUSE_ILLEGAL_INSTRUCTION &&
                           decode_instruction(trap.tval, &instr) == DR_OK;
    if (has_instr) {
        format_instruction(instr, FORMAT_PSEUDO, instr_str);
    }

    const struct TrapDetail d = trap.detail;
    switch (d.kind) {
        case DETAIL_NONE:
            break;
        case DETAIL_DECODE:
            append(out, &n, ": decode failed: %s", decode_str(d.as.decode));
            break;
        case DETAIL_CSR: {
            const char* name = csr_name(d.as.csr.index);
            append(out, &n, ": `%s`: %s of ", has_instr ? instr_str : "?",
                   d.as.csr.access == CSR_ACCESS_READ ? "read" : "write");
            if (name != NULL) {
                append(out, &n, "%s", name);
            } else {
                append(out, &n, "CSR 0x%03x", (unsigned)d.as.csr.index);
            }
            append(out, &n, " refused: %s", csr_status_str(d.as.csr.status));
            break;
        }
        case DETAIL_UNIMPLEMENTED:
            append(out, &n, ": `%s` decodes but is not implemented",
                   has_instr ? instr_str : "?");
            break;
        case DETAIL_BUS:
            append(out, &n, ": %s", bus_status_str(d.as.bus));
            break;
        case DETAIL_PRIVILEGED:
            append(out, &n, ": `%s` requires %s-mode, hart is in %s-mode",
                   has_instr ? instr_str : "?",
                   mode_str(d.as.privilege.required),
                   mode_str(d.as.privilege.current));
            break;
    }
}
