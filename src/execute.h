#pragma once

#include <stdbool.h>

#include "bus.h"
#include "hart.h"
#include "instruction.h"
#include "trap.h"

enum TrapStatus execute(struct Hart hart[static 1],
                        struct Bus bus[static 1],
                        struct Instruction instr,
                        struct Trap trap_out[static 1]  // written on trap
);
