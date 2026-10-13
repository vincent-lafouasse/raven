#include "execute.h"

enum ExecStatus execute(struct Cpu cpu[static 1],
                        struct Bus bus[static 1],
                        struct Instruction instr)
{
    (void)bus;

    const u32 pc = cpu->pc;
    u32 next_pc = pc + 4;  // only mutated on branches

    switch (instr.mnemonic) {
        case OP_ADDI:
            cpu->x[instr.rd] = cpu->x[instr.rs1] + (u32)instr.imm;
            break;
        case OP_BNE:
            if (cpu->x[instr.rs1] != cpu->x[instr.rs2]) {
                next_pc = pc + instr.imm;
            }
            break;
        default:
            return EXEC_UNIMPLEMENTED;
    }

    cpu->x[REG_ZERO] = 0u;
    cpu->pc = next_pc;
    return EXEC_OK;
}
