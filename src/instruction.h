#pragma once

#include "types.h"

enum Mnemonic {
    // integer computation
    OP_ADD,
    OP_SUB,
    OP_XOR,
    OP_OR,
    OP_AND,
    OP_SLL,
    OP_SRL,
    OP_SRA,
    OP_SLT,
    OP_SLTU,

    // integer computation with immediate
    OP_ADDI,
    OP_XORI,
    OP_ORI,
    OP_ANDI,
    OP_SLLI,
    OP_SRLI,
    OP_SRAI,
    OP_SLTI,
    OP_SLTIU,

    // Load/Store
    OP_LB,
    OP_LH,
    OP_LW,
    OP_LBU,
    OP_LHU,
    OP_SB,
    OP_SH,
    OP_SW,

    // branch/jump
    OP_BEQ,
    OP_BNE,
    OP_BLT,
    OP_BGE,
    OP_BLTU,
    OP_BGEU,
    OP_JAL,
    OP_JALR,

    // upper immediate
    OP_LUI,
    OP_AUIPC,

    // system
    OP_ECALL,
    OP_EBREAK,

    // Zicsr
    OP_CSRRW,
    OP_CSRRS,
    OP_CSRRC,
    OP_CSRRWI,
    OP_CSRRSI,
    OP_CSRRCI,

    // memory ordering
    // imm holds the raw, zero-extended bits 31:20: fm (11:8), pred (7:4),
    // succ (3:0), each set ordered i, o, r, w from msb to lsb
    OP_FENCE,
};

struct Instruction {
    u32 raw;
    enum Mnemonic mnemonic;

    // payload, some fields might be garbage
    // not worth the effort to make this an union
    u8 rd, rs1, rs2;
    i32 imm;
    u16 csr;
};

enum DecodeResult {
    DR_OK = 0,
    DR_BAD_LENGTH,
    DR_BAD_OPCODE,
    DR_BAD_FUNCT3,
    DR_BAD_FUNCT7,
    DR_UNSUPPORTED,
};

enum DecodeResult decode_instruction(u32 word,
                                     struct Instruction out[static 1]);

// ABI name of integer register x0..x31 ("zero", "ra", "sp", ...)
const char* reg_name(u8 reg);

// enough for any formatted instruction, including the `.word` fallback
#define INSTRUCTION_STR_LEN 128

void format_instruction(struct Instruction instr,
                        char out[static INSTRUCTION_STR_LEN]);
