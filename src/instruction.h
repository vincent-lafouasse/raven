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

enum InstructionKind {
    IK_STORE,
    IK_BRANCH,
    IK_REGISTER_REGISTER,
    IK_IMMEDIATE,
    IK_UPPER_IMMEDIATE,
    IK_JUMP,
};

struct StoreInstruction {
    u8 rs1, rs2, funct3;
    i32 imm; /* s12 */
};

struct BranchInstruction {
    u8 rs1, rs2, funct3;
    i32 imm; /* s13, bit 0 always 0 */
};

struct RegisterRegisterInstruction {
    u8 rd, rs1, rs2, funct3, funct7;
};

struct ImmediateInstruction {
    u8 rd, rs1, funct3;
    i32 imm; /* s12 */
};

struct UpperImmediateInstruction {
    u8 rd;
    i32 imm; /* already shifted: low 12 bits are zero */
};

struct JumpInstruction {
    u8 rd;
    i32 imm; /* s21, bit 0 always 0 */
};

struct Instruction {
    u8 opcode;
    enum InstructionKind kind;
    union {
        struct StoreInstruction store;
        struct BranchInstruction branch;
        struct RegisterRegisterInstruction reg_reg;
        struct ImmediateInstruction imm;
        struct UpperImmediateInstruction upper;
        struct JumpInstruction jump;
    } as;
};

struct Instruction decode_instruction(u32 raw_instruction);
