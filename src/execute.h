#pragma once

#include <stdbool.h>

#include "bus.h"
#include "cpu.h"
#include "instruction.h"
#include "trap.h"

struct ExecStatus {
    bool trapped;
    struct Trap trap;
};

struct ExecStatus execute(struct Cpu cpu[static 1],
                          struct Bus bus[static 1],
                          struct Instruction instr);
