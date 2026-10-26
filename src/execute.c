#include "execute.h"

#include "debug.h"

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

// sign extension from 8 and 16 bits, in u32 arithmetic: flipping the sign bit
// then subtracting it borrows through all the upper bits when it was set
static u32 sext8(u32 value)
{
    return ((value & 0xffu) ^ 0x80u) - 0x80u;
}

static u32 sext16(u32 value)
{
    return ((value & 0xffffu) ^ 0x8000u) - 0x8000u;
}

static u32 access_size(enum Mnemonic m)
{
    switch (m) {
        case OP_LB:
        case OP_LBU:
        case OP_SB:
            return 1;
        case OP_LH:
        case OP_LHU:
        case OP_SH:
            return 2;
        case OP_LW:
        case OP_SW:
            return 4;
        default:
            UNREACHABLE();
    }
}

// The hart's load/store unit: performs a data access over the bus on behalf
// of an instruction and reports failures as traps. Address translation and
// permission checks (Sv32, PMP) will go here.
//
// On TRAP_RAISED nothing was read or written: *out is untouched.

enum AccessType {
    ACCESS_LOAD,
    ACCESS_STORE,
};

static struct Trap data_fault(enum BusStatus status,
                              enum AccessType access,
                              u32 addr)
{
    const bool misaligned = status == BUS_MISALIGNED;

    switch (access) {
        case ACCESS_LOAD:
            return make_trap(
                misaligned ? CAUSE_MISALIGNED_LOAD : CAUSE_LOAD_ACCESS, addr);
        case ACCESS_STORE:
            return make_trap(
                misaligned ? CAUSE_MISALIGNED_STORE : CAUSE_STORE_ACCESS, addr);
    }
    UNREACHABLE();
}

// Reads `size` (1, 2 or 4) bytes at `addr`, zero-extended into *out. Sign
// extension (lb, lh) is the instruction's job, not the LSU's.
static enum TrapStatus mem_load(struct Hart hart[static 1],
                                struct Bus bus[static 1],
                                u32 addr,
                                u32 size,
                                u32 out[static 1],
                                struct Trap trap_out[static 1])
{
    (void)hart;  // translation and permissions will need its CSRs

    u32 value;
    const enum BusStatus status = bus_read(bus, addr, size, &value);
    if (status != BUS_OK) {
        *trap_out = data_fault(status, ACCESS_LOAD, addr);
        return TRAP_RAISED;
    }

    *out = value;
    return TRAP_NONE;
}

// Writes the low `size` (1, 2 or 4) bytes of `value` at `addr`.
static enum TrapStatus mem_store(struct Hart hart[static 1],
                                 struct Bus bus[static 1],
                                 u32 addr,
                                 u32 size,
                                 u32 value,
                                 struct Trap trap_out[static 1])
{
    (void)hart;  // translation and permissions will need its CSRs

    const enum BusStatus status = bus_write(bus, addr, size, value);
    if (status != BUS_OK) {
        *trap_out = data_fault(status, ACCESS_STORE, addr);
        return TRAP_RAISED;
    }

    return TRAP_NONE;
}

static enum TrapStatus exec_csr(struct Hart hart[static 1],
                                struct Instruction instr,
                                struct Trap trap_out[static 1])
{
    const bool immediate = instr.mnemonic == OP_CSRRWI ||
                           instr.mnemonic == OP_CSRRSI ||
                           instr.mnemonic == OP_CSRRCI;
    const u32 operand = immediate ? (u32)instr.imm : hart->x[instr.rs1];
    const bool operand_is_zero_reg =
        immediate ? instr.imm == 0 : instr.rs1 == REG_ZERO;

    const bool is_rw =
        instr.mnemonic == OP_CSRRW || instr.mnemonic == OP_CSRRWI;
    const bool is_rs =
        instr.mnemonic == OP_CSRRS || instr.mnemonic == OP_CSRRSI;

    const bool reads = !is_rw || instr.rd != REG_ZERO;
    const bool writes = is_rw || !operand_is_zero_reg;

    // every refusal (nonexistent, read-only, privilege) is the same
    // architectural outcome: illegal instruction, tval = the instruction
    const struct Trap illegal = make_trap(CAUSE_ILLEGAL_INSTRUCTION, instr.raw);

    u32 old = 0;
    if (reads && csr_read(instr.csr, &old, hart->mode, hart->csr) != CSR_OK) {
        *trap_out = illegal;
        return TRAP_RAISED;
    }

    if (writes) {
        u32 new_value;
        if (is_rw) {
            new_value = operand;
        } else if (is_rs) {
            new_value = old | operand;
        } else {
            new_value = old & ~operand;
        }

        // reads have no side effects yet, so failing here after a
        // successful read still leaves the hart untouched
        if (csr_write(instr.csr, new_value, hart->mode, &hart->csr) != CSR_OK) {
            *trap_out = illegal;
            return TRAP_RAISED;
        }
    }

    // operand was captured above, so rd == rs1 is fine
    if (reads) {
        hart->x[instr.rd] = old;
    }
    return TRAP_NONE;
}

enum TrapStatus execute(struct Hart hart[static 1],
                        struct Bus bus[static 1],
                        struct Instruction instr,
                        struct Trap trap_out[static 1])
{
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
        // the address is computed from rs1 before rd is written, so
        // `lw x5, 0(x5)` works. A load into x0 still happens (and can still
        // fault), its result is discarded when x0 is reset below.
        case OP_LB:
        case OP_LBU:
        case OP_LH:
        case OP_LHU:
        case OP_LW: {
            const u32 addr = hart->x[rs1] + (u32)imm;
            u32 value;
            if (mem_load(hart, bus, addr, access_size(instr.mnemonic), &value,
                         trap_out) == TRAP_RAISED) {
                return TRAP_RAISED;  // rd untouched, pc unchanged
            }
            if (instr.mnemonic == OP_LB) {
                value = sext8(value);
            } else if (instr.mnemonic == OP_LH) {
                value = sext16(value);
            }
            hart->x[rd] = value;
            break;
        }
        // mem_store only writes the low `size` bytes of rs2
        case OP_SB:
        case OP_SH:
        case OP_SW: {
            const u32 addr = hart->x[rs1] + (u32)imm;
            if (mem_store(hart, bus, addr, access_size(instr.mnemonic),
                          hart->x[rs2], trap_out) == TRAP_RAISED) {
                return TRAP_RAISED;
            }
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
        case OP_CSRRW:
        case OP_CSRRS:
        case OP_CSRRC:
        case OP_CSRRWI:
        case OP_CSRRSI:
        case OP_CSRRCI:
            if (exec_csr(hart, instr, trap_out) == TRAP_RAISED) {
                return TRAP_RAISED;
            }
            break;
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
