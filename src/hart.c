#include "hart.h"

#include <stdio.h>

#include "bus.h"
#include "execute.h"

void hart_reset(struct Hart hart[static 1], u32 entry)
{
    *hart = (struct Hart){0};

    hart->x[REG_SP] = RAM_BASE + RAM_SIZE;

    hart->pc = entry;

    hart->mode = MODE_MACHINE;

    hart->csr = csr_new();
}

enum TrapStatus hart_fetch(struct Hart hart[static 1],
                           struct Bus bus[static 1],
                           struct Instruction instr_out[static 1],
                           struct Trap trap_out[static 1])
{
    u32 raw_instruction;
    switch (bus_read(bus, hart->pc, 4, &raw_instruction)) {
        case BUS_OK:
            break;
        case BUS_UNMAPPED:
            *trap_out = trap_bus(CAUSE_FETCH_ACCESS, hart->pc, BUS_UNMAPPED);
            return TRAP_RAISED;
        case BUS_MISALIGNED:
            *trap_out =
                trap_bus(CAUSE_MISALIGNED_FETCH, hart->pc, BUS_MISALIGNED);
            return TRAP_RAISED;
    }

    struct Instruction instruction;
    const enum DecodeResult decode_status =
        decode_instruction(raw_instruction, &instruction);
    if (decode_status != DR_OK) {
        *trap_out = trap_decode(raw_instruction, decode_status);
        return TRAP_RAISED;
    }

    *instr_out = instruction;
    return TRAP_NONE;
}

enum TrapStatus hart_execute(struct Hart hart[static 1],
                             struct Bus bus[static 1],
                             struct Instruction instr,
                             struct Trap trap_out[static 1])
{
    return execute(hart, bus, instr, trap_out);
}

void hart_dump(const struct Hart hart[static 1], char out[static HART_DUMP_LEN])
{
    const size_t cap = HART_DUMP_LEN;
    size_t n = 0;

    // each piece is a fixed width well below what's left, so n stays < cap
    n += (size_t)snprintf(out + n, cap - n, "pc   0x%08x\n", hart->pc);

    for (unsigned i = 0; i < 32; ++i) {
        const char* sep = (i % 4 == 3) ? "\n" : "  ";
        n += (size_t)snprintf(out + n, cap - n, "x%-2u %-4s 0x%08x%s", i,
                              reg_name((u8)i), hart->x[i], sep);
    }
}
