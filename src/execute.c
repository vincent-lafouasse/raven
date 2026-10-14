#include "execute.h"

static struct ExecStatus exec_trap(enum TrapCause cause, u32 tval)
{
    return (struct ExecStatus){.trapped = true, .trap = make_trap(cause, val)};
}

static struct ExecStatus exec_ok(void)
{
    return (struct ExecStatus){.trapped = bool};
}

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
    u32 next_pc = pc + 4;  // only mutated by branches and jumps

    switch (instr.mnemonic) {
        case OP_ADD:
            cpu->x[rd] = cpu->x[rs1] + cpu->x[rs2];
            break;
        case OP_ADDI:
            cpu->x[rd] = cpu->x[rs1] + (u32)imm;
            break;
        case OP_SUB:
            cpu->x[rd] = cpu->x[rs1] - cpu->x[rs2];
            break;
        case OP_AND:
            cpu->x[rd] = cpu->x[rs1] & cpu->x[rs2];
            break;
        case OP_ANDI:
            cpu->x[rd] = cpu->x[rs1] & (u32)imm;
            break;
        case OP_OR:
            cpu->x[rd] = cpu->x[rs1] | cpu->x[rs2];
            break;
        case OP_ORI:
            cpu->x[rd] = cpu->x[rs1] | (u32)imm;
            break;
        case OP_XOR:
            cpu->x[rd] = cpu->x[rs1] ^ cpu->x[rs2];
            break;
        case OP_XORI:
            cpu->x[rd] = cpu->x[rs1] ^ (u32)imm;
            break;
        // shifts by register use only the low 5 bits of rs2. The immediate
        // forms carry a 5-bit shamt, already extracted by the decoder.
        case OP_SLL:
            cpu->x[rd] = cpu->x[rs1] << (cpu->x[rs2] & 0x1f);
            break;
        case OP_SLLI:
            cpu->x[rd] = cpu->x[rs1] << (u32)imm;
            break;
        case OP_SRL:
            cpu->x[rd] = cpu->x[rs1] >> (cpu->x[rs2] & 0x1f);
            break;
        case OP_SRLI:
            cpu->x[rd] = cpu->x[rs1] >> (u32)imm;
            break;
        case OP_SRA:
            cpu->x[rd] = sra32(cpu->x[rs1], cpu->x[rs2] & 0x1f);
            break;
        case OP_SRAI:
            cpu->x[rd] = sra32(cpu->x[rs1], (u32)imm);
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
        case OP_ECALL:
            return exec_trap(CAUSE_ECALL_M, 0);
        case OP_EBREAK:
            return exec_trap(CAUSE_BREAKPOINT, pc);
        default:
            return exec_trap(CAUSE_ILLEGAL_INSTRUCTION, instr.raw);
    }

    // without the C extension, every instruction is 4-byte aligned. The
    // fault belongs to the branch or jump, so check before committing.
    if (next_pc % 4 != 0) {
        return exec_trap(CAUSE_MISALIGNED_FETCH, next_pc);
    }

    cpu->x[REG_ZERO] = 0u;
    cpu->pc = next_pc;
    return exec_ok();
}
