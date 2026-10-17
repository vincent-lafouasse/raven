#include "hart.h"

#include <stdio.h>

#include "bus.h"
#include "execute.h"

void hart_reset(struct Hart hart[static 1], u32 entry)
{
    *hart = (struct Hart){0};

    hart->x[REG_SP] = RAM_BASE + RAM_SIZE;

    hart->pc = entry;
}

enum TrapStatus hart_tick(struct Hart hart[static 1],
                          struct Bus bus[static 1],
                          struct Trap trap_out[static 1]  // written on trap
)
{
    u32 raw_instruction;
    switch (bus_read(bus, hart->pc, 4, &raw_instruction)) {
        case BUS_OK:
            break;
        case BUS_UNMAPPED:
            *trap_out = make_trap(CAUSE_FETCH_ACCESS, hart->pc);
            return TRAP_RAISED;
        case BUS_MISALIGNED:
            *trap_out = make_trap(CAUSE_MISALIGNED_FETCH, hart->pc);
            return TRAP_RAISED;
    }

    struct Instruction instruction;
    const enum DecodeResult decode_status =
        decode_instruction(raw_instruction, &instruction);
    if (decode_status != DR_OK) {
        // might store decode status for diagnostic but for now it's all just
        // Illegal Instruction
        *trap_out = make_trap(CAUSE_ILLEGAL_INSTRUCTION, raw_instruction);
        return TRAP_RAISED;
    }

    return execute(hart, bus, instruction, trap_out);
}

static const char* const k_abi_names[32] = {
    "zero", "ra", "sp", "gp", "tp",  "t0",  "t1", "t2", "s0", "s1", "a0",
    "a1",   "a2", "a3", "a4", "a5",  "a6",  "a7", "s2", "s3", "s4", "s5",
    "s6",   "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6",
};

void hart_dump(const struct Hart hart[static 1], char out[static HART_DUMP_LEN])
{
    const size_t cap = HART_DUMP_LEN;
    size_t n = 0;

    // each piece is a fixed width well below what's left, so n stays < cap
    n += (size_t)snprintf(out + n, cap - n, "pc   0x%08x\n", hart->pc);

    for (unsigned i = 0; i < 32; ++i) {
        const char* sep = (i % 4 == 3) ? "\n" : "  ";
        n += (size_t)snprintf(out + n, cap - n, "x%-2u %-4s 0x%08x%s", i,
                              k_abi_names[i], hart->x[i], sep);
    }
}
