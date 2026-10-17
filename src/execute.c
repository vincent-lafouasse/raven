#include "execute.h"

// arithmetic right shift without relying on implementation-defined signed >>
static u32 sra32(u32 value, u32 shamt)
{
    const u32 shifted = value >> shamt;
    if ((value & 0x80000000u) == 0) {
        return shifted;
    }
    // fill the top `shamt` bits with ones
    return shifted | ~(UINT32_MAX >> shamt);
}

enum TrapStatus execute(struct Hart hart[static 1],
                        struct Bus bus[static 1],
                        struct Instruction instr,
                        struct Trap trap_out[static 1])
{
    (void)bus;

    const u8 rd = instr.rd;
    const u8 rs1 = instr.rs1;
    const u8 rs2 = instr.rs2;
    const i32 imm = instr.imm;

    const u32 pc = hart->pc;
    u32 next_pc = pc + 4;  // only mutated by branches and jumps

    switch (instr.mnemonic) {
        case OP_ADD:
            hart->x[rd] = hart->x[rs1] + hart->x[rs2];
            break;
        case OP_ADDI:
            hart->x[rd] = hart->x[rs1] + (u32)imm;
            break;
        case OP_SUB:
            hart->x[rd] = hart->x[rs1] - hart->x[rs2];
            break;
        case OP_AND:
            hart->x[rd] = hart->x[rs1] & hart->x[rs2];
            break;
        case OP_ANDI:
            hart->x[rd] = hart->x[rs1] & (u32)imm;
            break;
        case OP_OR:
            hart->x[rd] = hart->x[rs1] | hart->x[rs2];
            break;
        case OP_ORI:
            hart->x[rd] = hart->x[rs1] | (u32)imm;
            break;
        case OP_XOR:
            hart->x[rd] = hart->x[rs1] ^ hart->x[rs2];
            break;
        case OP_XORI:
            hart->x[rd] = hart->x[rs1] ^ (u32)imm;
            break;
        // shifts by register use only the low 5 bits of rs2. The immediate
        // forms carry a 5-bit shamt, already extracted by the decoder.
        case OP_SLL:
            hart->x[rd] = hart->x[rs1] << (hart->x[rs2] & 0x1f);
            break;
        case OP_SLLI:
            hart->x[rd] = hart->x[rs1] << (u32)imm;
            break;
        case OP_SRL:
            hart->x[rd] = hart->x[rs1] >> (hart->x[rs2] & 0x1f);
            break;
        case OP_SRLI:
            hart->x[rd] = hart->x[rs1] >> (u32)imm;
            break;
        case OP_SRA:
            hart->x[rd] = sra32(hart->x[rs1], hart->x[rs2] & 0x1f);
            break;
        case OP_SRAI:
            hart->x[rd] = sra32(hart->x[rs1], (u32)imm);
            break;
        case OP_SLT:
            hart->x[rd] = (i32)hart->x[rs1] < (i32)hart->x[rs2];
            break;
        case OP_SLTU:
            hart->x[rd] = hart->x[rs1] < hart->x[rs2];
            break;
        case OP_SLTI:
            hart->x[rd] = (i32)hart->x[rs1] < imm;
            break;
        case OP_SLTIU:
            hart->x[rd] = hart->x[rs1] < (u32)imm;
            break;
        case OP_BNE:
            if (hart->x[rs1] != hart->x[rs2]) {
                next_pc = pc + (u32)imm;
            }
            break;
        case OP_BEQ:
            if (hart->x[rs1] == hart->x[rs2]) {
                next_pc = pc + (u32)imm;
            }
            break;
        case OP_BGE:
            if ((i32)hart->x[rs1] >= (i32)hart->x[rs2]) {
                next_pc = pc + (u32)imm;
            }
            break;
        case OP_BGEU:
            if (hart->x[rs1] >= hart->x[rs2]) {
                next_pc = pc + (u32)imm;
            }
            break;
        case OP_BLT:
            if ((i32)hart->x[rs1] < (i32)hart->x[rs2]) {
                next_pc = pc + (u32)imm;
            }
            break;
        case OP_BLTU:
            if (hart->x[rs1] < hart->x[rs2]) {
                next_pc = pc + (u32)imm;
            }
            break;
        case OP_JAL:
            hart->x[rd] = next_pc;
            next_pc = pc + (u32)imm;
            break;
        case OP_JALR: {
            const u32 target = hart->x[rs1] + (u32)imm;
            hart->x[rd] = next_pc;
            next_pc = target;
            break;
        }
        case OP_LUI:
            hart->x[rd] = (u32)imm;
            break;
        case OP_AUIPC:
            hart->x[rd] = pc + (u32)imm;
            break;
        case OP_FENCE:
            // fence is a no-op, all this is single threaded
            break;
        case OP_ECALL:
            *trap_out = make_trap(CAUSE_ECALL_M, 0);
            return TRAP_RAISED;
        case OP_EBREAK:
            *trap_out = make_trap(CAUSE_BREAKPOINT, pc);
            return TRAP_RAISED;
        default:
            *trap_out = make_trap(CAUSE_ILLEGAL_INSTRUCTION, instr.raw);
            return TRAP_RAISED;
    }

    // without the C extension, every instruction is 4-byte aligned. The
    // fault belongs to the branch or jump, so check before committing.
    if (next_pc % 4 != 0) {
        *trap_out = make_trap(CAUSE_MISALIGNED_FETCH, next_pc);
        return TRAP_RAISED;
    }

    hart->x[REG_ZERO] = 0u;
    hart->pc = next_pc;
    return TRAP_NONE;
}
