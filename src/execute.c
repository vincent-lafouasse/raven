#include "execute.h"

enum ExecStatus execute(struct Cpu cpu[static 1],
                        struct Bus bus[static 1],
                        struct Instruction instr)
{
    (void)bus;

    switch (instr.mnemonic) {
        case OP_ADDI:
            cpu->x[instr.rd] = cpu->x[instr.rs1] + (u32)instr.imm;
            goto no_branch;
        default:
            return EXEC_UNIMPLEMENTED;
    }

no_branch:
    cpu->pc += 4;
    return EXEC_OK;
}
