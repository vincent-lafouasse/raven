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
    u8 funct3;
    u8 rs1, rs2;
    u16 imm;
};

struct BranchInstruction {};

struct Instruction {
    u8 opcode;
    enum InstructionKind kind;
    union {
        struct StoreInstruction store;
        struct BranchInstruction branch;
    } as;
};

struct Instruction decode_instruction(u32 raw_instruction);

int main(void)
{
    return 0;
}
