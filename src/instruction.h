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
};

struct Instruction {
    enum Mnemonic mnemonic;

    // payload, some fields might be garbage
    u8 rd, rs1, rs2;
    i32 imm;
};

enum DecodeResult {
    DR_OK = 0,
    DR_BAD_LENGTH,  // low two bits are not 0b11 (compressed or garbage)
    DR_BAD_OPCODE,  // major opcode (bits 6:0) is not one you know
    DR_BAD_FUNCT3,  // valid opcode, funct3 not defined for it
    DR_BAD_FUNCT7,  // valid opcode/funct3, funct7 (or shift-imm upper bits) not
                    // defined
    DR_UNSUPPORTED,  // legal RISC-V that you chose not to implement (FENCE,
                     // ECALL, ...)
};

enum DecodeResult decode_instruction(u32 word,
                                     struct Instruction out[static 1]);
