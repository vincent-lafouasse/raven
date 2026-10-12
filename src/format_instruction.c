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
    // integer computation
    [OP_ADD] = {"add", OPF_REG_REG},
    [OP_SUB] = {"sub", OPF_REG_REG},
    [OP_XOR] = {"xor", OPF_REG_REG},
    [OP_OR] = {"or", OPF_REG_REG},
    [OP_AND] = {"and", OPF_REG_REG},
    [OP_SLL] = {"sll", OPF_REG_REG},
    [OP_SRL] = {"srl", OPF_REG_REG},
    [OP_SRA] = {"sra", OPF_REG_REG},
    [OP_SLT] = {"slt", OPF_REG_REG},
    [OP_SLTU] = {"sltu", OPF_REG_REG},

    // integer computation with immediate
    [OP_ADDI] = {"addi", OPF_REG_IMM},
    [OP_XORI] = {"xori", OPF_REG_IMM},
    [OP_ORI] = {"ori", OPF_REG_IMM},
    [OP_ANDI] = {"andi", OPF_REG_IMM},
    [OP_SLLI] = {"slli", OPF_REG_IMM},
    [OP_SRLI] = {"srli", OPF_REG_IMM},
    [OP_SRAI] = {"srai", OPF_REG_IMM},
    [OP_SLTI] = {"slti", OPF_REG_IMM},
    [OP_SLTIU] = {"sltiu", OPF_REG_IMM},

    // load/store
    [OP_LB] = {"lb", OPF_LOAD},
    [OP_LH] = {"lh", OPF_LOAD},
    [OP_LW] = {"lw", OPF_LOAD},
    [OP_LBU] = {"lbu", OPF_LOAD},
    [OP_LHU] = {"lhu", OPF_LOAD},
    [OP_SB] = {"sb", OPF_STORE},
    [OP_SH] = {"sh", OPF_STORE},
    [OP_SW] = {"sw", OPF_STORE},

    // branch/jump
    [OP_BEQ] = {"beq", OPF_BRANCH},
    [OP_BNE] = {"bne", OPF_BRANCH},
    [OP_BLT] = {"blt", OPF_BRANCH},
    [OP_BGE] = {"bge", OPF_BRANCH},
    [OP_BLTU] = {"bltu", OPF_BRANCH},
    [OP_BGEU] = {"bgeu", OPF_BRANCH},
    [OP_JAL] = {"jal", OPF_JUMP},
    [OP_JALR] = {"jalr", OPF_LOAD},  // I-format, printed as imm(rs1)

    // upper immediate
    [OP_LUI] = {"lui", OPF_UPPER_IMM},
    [OP_AUIPC] = {"auipc", OPF_UPPER_IMM},

    // system
    [OP_ECALL] = {"ecall", OPF_NONE},
    [OP_EBREAK] = {"ebreak", OPF_NONE},
};

void format_instruction(struct Instruction instr, char out[static 128])
{
    const char* name = k_info[instr.mnemonic].name;
    const enum OperandForm form = k_info[instr.mnemonic].form;

    switch (form) {
        case OPF_REG_REG:
            snprintf(out, 128, "%s x%d, x%d, x%d", name, instr.rd, instr.rs1,
                     instr.rs2);
            break;
        case OPF_REG_IMM:
            snprintf(out, 128, "%s x%d, x%d, %d", name, instr.rd, instr.rs1,
                     instr.imm);
            break;
        case OPF_LOAD:
            snprintf(out, 128, "%s x%d, %d(x%d)", name, instr.rd, instr.imm,
                     instr.rs1);
            break;
        case OPF_STORE:
            snprintf(out, 128, "%s x%d, %d(x%d)", name, instr.rs2, instr.imm,
                     instr.rs1);
            break;
        case OPF_BRANCH:
            snprintf(out, 128, "%s x%d, x%d, .%+d", name, instr.rs1, instr.rs2,
                     instr.imm);
            break;
        case OPF_UPPER_IMM:
            // stored imm is already shifted, the assembler wants the 20-bit
            // value
            snprintf(out, 128, "%s x%d, 0x%x", name, instr.rd,
                     (unsigned)instr.imm >> 12);
            break;
        case OPF_JUMP:
            snprintf(out, 128, "%s x%d, .%+d", name, instr.rd, instr.imm);
            break;
        case OPF_NONE:
            snprintf(out, 128, "%s", name);
            break;
    }
}
