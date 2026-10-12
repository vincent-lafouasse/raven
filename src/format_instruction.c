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
}
