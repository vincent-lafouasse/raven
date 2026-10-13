#include "execute.h"

enum ExecStatus execute(struct Cpu cpu[static 1],
                        struct Bus bus[static 1],
                        struct Instruction instr)
{
    (void)cpu;
    (void)bus;
    (void)instr;
    return EXEC_UNIMPLEMENTED;
}
