#include "execute.h"

enum ExecStatus execute(struct Cpu cpu[static 1],
                        struct Bus bus[static 1],
                        struct Instruction instr)
{
    (void)bus;

    const u8 rd = instr.rd;
    const u8 rs1 = instr.rs1;
    const u8 rs2 = instr.rs2;
    const i32 imm = instr.imm;

    const u32 pc = cpu->pc;
    u32 next_pc = pc + 4;  // only mutated on branches

    switch (instr.mnemonic) {
        case OP_ADDI:
            cpu->x[rd] = cpu->x[rs1] + (u32)imm;
            break;
        case OP_BNE:
            if (cpu->x[rs1] != cpu->x[rs2]) {
                next_pc = pc + imm;
            }
            break;
        case OP_LUI:
            cpu->x[rd] = imm;
            break;
        case OP_FENCE:
            // fence is a no-op, all this is single threaded
            break;
        default:
            return EXEC_UNIMPLEMENTED;
    }

    cpu->x[REG_ZERO] = 0u;
    cpu->pc = next_pc;
    return EXEC_OK;
}
