#pragma once

#include "bus.h"
#include "cpu.h"
#include "instruction.h"

enum ExecStatus {
    EXEC_OK,
    EXEC_ECALL,
    EXEC_EBREAK,
    EXEC_BUS_FAULT,
    EXEC_MISALIGNED_PC,
    EXEC_UNSUPPORTED,
    EXEC_UNIMPLEMENTED, // temporary
};

enum ExecStatus execute(struct Cpu cpu[static 1],
                        struct Bus bus[static 1],
                        struct Instruction instr);
