#include "types.h"

enum InstructionKind {
    IK_STORE,
    IK_BRANCH,
    IK_REGISTER_REGISTER,
    IK_IMMEDIATE,
    IK_UPPER_IMMEDIATE,
    IK_JUMP,
};

struct StoreInstruction {};
struct BranchInstruction {};

struct Instruction {
    enum InstructionKind kind;
    union {
        struct StoreInstruction store;
        struct BranchInstruction branch;
    } as;
};

int main(void)
{
    return 0;
}
