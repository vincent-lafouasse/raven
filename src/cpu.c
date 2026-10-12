#include "cpu.h"

#include "bus.h"

void cpu_reset(struct Cpu cpu[static 1], u32 entry)
{
    *cpu = (struct Cpu){0};

    cpu->x[REG_SP] = RAM_BASE + RAM_SIZE;

    cpu->pc = entry;
}
