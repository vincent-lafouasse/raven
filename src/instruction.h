#pragma once

#include "types.h"

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
