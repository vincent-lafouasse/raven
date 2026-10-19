#include "instruction.h"

#include <assert.h>
#include <stdio.h>

#include "csr.h"

enum OperandForm {
    OPF_REG_REG,
    OPF_REG_IMM,
    OPF_LOAD,
    OPF_STORE,
    OPF_BRANCH,
    OPF_UPPER_IMM,
    OPF_JUMP,
    OPF_NONE,
    OPF_FENCE,
    OPF_CSR_REG,
    OPF_CSR_IMM,
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

    // Zicsr
    [OP_CSRRW] = {"csrrw", OPF_CSR_REG},
    [OP_CSRRS] = {"csrrs", OPF_CSR_REG},
    [OP_CSRRC] = {"csrrc", OPF_CSR_REG},
    [OP_CSRRWI] = {"csrrwi", OPF_CSR_IMM},
    [OP_CSRRSI] = {"csrrsi", OPF_CSR_IMM},
    [OP_CSRRCI] = {"csrrci", OPF_CSR_IMM},

    // memory ordering
    [OP_FENCE] = {"fence", OPF_FENCE},
};

static const char* const k_abi_names[32] = {
    "zero", "ra", "sp", "gp", "tp",  "t0",  "t1", "t2", "s0", "s1", "a0",
    "a1",   "a2", "a3", "a4", "a5",  "a6",  "a7", "s2", "s3", "s4", "s5",
    "s6",   "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6",
};

const char* reg_name(u8 reg)
{
    assert(reg < 32);
    return k_abi_names[reg];
}

// writes a fence predecessor/successor set as a subset of "iorw"
static void format_fence_set(u32 set, char out[static 5])
{
    const char* letters = "iorw";
    size_t n = 0;

    for (u32 i = 0; i < 4; ++i) {
        if (set & (0x8u >> i)) {
            out[n++] = letters[i];
        }
    }
    out[n] = '\0';
}

static void format_fence(struct Instruction instr,
                         char out[static INSTRUCTION_STR_LEN])
{
    const u32 fm = ((u32)instr.imm >> 8) & 0xf;
    const u32 pred = ((u32)instr.imm >> 4) & 0xf;
    const u32 succ = (u32)instr.imm & 0xf;

    if (instr.rd == 0 && instr.rs1 == 0) {
        if (fm == 0x8 && pred == 0x3 && succ == 0x3) {
            snprintf(out, INSTRUCTION_STR_LEN, "fence.tso");
            return;
        }
        if (fm == 0 && pred != 0 && succ != 0) {
            char p[5], q[5];
            format_fence_set(pred, p);
            format_fence_set(succ, q);
            snprintf(out, INSTRUCTION_STR_LEN, "fence %s, %s", p, q);
            return;
        }
    }

    // anything else (reserved fm, empty sets such as PAUSE, nonzero rd/rs1)
    // has no portable assembly syntax: rebuild the word so the output still
    // reassembles to the same bytes
    const u32 word = ((u32)instr.imm & 0xfffu) << 20 | (u32)instr.rs1 << 15 |
                     (u32)instr.rd << 7 | 0x0fu;
    snprintf(out, INSTRUCTION_STR_LEN, ".word 0x%08x", word);
}

// fits the longest standard name ("mhpmcounter31h") and "0xfff"
#define CSR_STR_LEN 16

// the standard name if there is one, so the trace reads `csrrw x0, mtvec,
// x5`; otherwise the raw address, which assemblers also accept
static void format_csr(u16 csr, char out[static CSR_STR_LEN])
{
    const char* name = csr_name(csr);

    if (name != NULL) {
        snprintf(out, CSR_STR_LEN, "%s", name);
    } else {
        snprintf(out, CSR_STR_LEN, "0x%03x", csr);
    }
}

void format_instruction(struct Instruction instr,
                        char out[static INSTRUCTION_STR_LEN])
{
    const char* name = k_info[instr.mnemonic].name;
    const enum OperandForm form = k_info[instr.mnemonic].form;

    switch (form) {
        case OPF_REG_REG:
            snprintf(out, INSTRUCTION_STR_LEN, "%s %s, %s, %s", name,
                     reg_name(instr.rd), reg_name(instr.rs1),
                     reg_name(instr.rs2));
            break;
        case OPF_REG_IMM:
            snprintf(out, INSTRUCTION_STR_LEN, "%s %s, %s, %d", name,
                     reg_name(instr.rd), reg_name(instr.rs1), instr.imm);
            break;
        case OPF_LOAD:
            snprintf(out, INSTRUCTION_STR_LEN, "%s %s, %d(%s)", name,
                     reg_name(instr.rd), instr.imm, reg_name(instr.rs1));
            break;
        case OPF_STORE:
            snprintf(out, INSTRUCTION_STR_LEN, "%s %s, %d(%s)", name,
                     reg_name(instr.rs2), instr.imm, reg_name(instr.rs1));
            break;
        case OPF_BRANCH:
            snprintf(out, INSTRUCTION_STR_LEN, "%s %s, %s, .%+d", name,
                     reg_name(instr.rs1), reg_name(instr.rs2), instr.imm);
            break;
        case OPF_UPPER_IMM:
            // stored imm is already shifted, the assembler wants the 20-bit
            // value
            snprintf(out, INSTRUCTION_STR_LEN, "%s %s, 0x%x", name,
                     reg_name(instr.rd), (u32)instr.imm >> 12);
            break;
        case OPF_JUMP:
            snprintf(out, INSTRUCTION_STR_LEN, "%s %s, .%+d", name,
                     reg_name(instr.rd), instr.imm);
            break;
        case OPF_NONE:
            snprintf(out, INSTRUCTION_STR_LEN, "%s", name);
            break;
        case OPF_FENCE:
            format_fence(instr, out);
            break;
        case OPF_CSR_REG: {
            char csr[CSR_STR_LEN];
            format_csr(instr.csr, csr);
            snprintf(out, INSTRUCTION_STR_LEN, "%s %s, %s, %s", name,
                     reg_name(instr.rd), csr, reg_name(instr.rs1));
            break;
        }
        case OPF_CSR_IMM: {
            char csr[CSR_STR_LEN];
            format_csr(instr.csr, csr);
            snprintf(out, INSTRUCTION_STR_LEN, "%s %s, %s, %d", name,
                     reg_name(instr.rd), csr, instr.imm);
            break;
        }
    }
}
