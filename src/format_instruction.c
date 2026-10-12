#include "instruction.h"

enum OperandForm {
    OPF_REG_REG,
    OPF_REG_IMM,
    OPF_LOAD,
    OPF_STORE,
    OPF_BRANCH,
    OPF_UPPER_IMM,
    OPF_JUMP,
    OPF_NONE,
};

static const struct {
    const char* name;
    enum OperandForm form;
} k_info[] = {
    [OP_ADD] = {"add", OPF_REG_REG},
};

void format_instruction(struct Instruction instr, char out[static 128])
{
    const char* name = k_info[instr.mnemonic].name;
    const enum OperandForm form = k_info[instr.mnemonic].form;

    // a guard against overflow
    // should never happen since 128 char is plenty enough for anything
    int n = -1;

    switch (form) {
        case OPF_REG_REG:
            n = snprintf(out, 128, "%s x%d, x%d, x%d", name, instr.rd,
                         instr.rs1, instr.rs2);
            break;
        case OPF_REG_IMM:
            n = snprintf(out, 128, "%s x%d, x%d, %d", name, instr.rd, instr.rs1,
                         instr.imm);
            break;
        case OPF_LOAD:
            n = snprintf(out, 128, "%s x%d, %d(x%d)", name, instr.rd, instr.imm,
                         instr.rs1);
            break;
        case OPF_STORE:
            n = snprintf(out, 128, "%s x%d, %d(x%d)", name, instr.rs2,
                         instr.imm, instr.rs1);
            break;
        case OPF_BRANCH:
            n = snprintf(out, 128, "%s x%d, x%d, .%+d", name, instr.rs1,
                         instr.rs2, instr.imm);
            break;
        case OPF_UPPER_IMM:
            // stored imm is already shifted, the assembler wants the 20-bit
            // value
            n = snprintf(out, 128, "%s x%d, 0x%x", name, instr.rd,
                         (unsigned)instr.imm >> 12);
            break;
        case OPF_JUMP:
            n = snprintf(out, 128, "%s x%d, .%+d", name, instr.rd, instr.imm);
            break;
        case OPF_NONE:
            n = snprintf(out, 128, "%s", name);
            break;
    }

    assert(n >= 0 && n < 128);
    (void)n;
}
