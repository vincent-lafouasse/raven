#include "instruction.h"

#include <assert.h>

enum Opcode {
    OPC_OP = 0x33,
    OPC_OP_IMM = 0x13,
    OPC_LOAD = 0x03,
    OPC_STORE = 0x23,
    OPC_BRANCH = 0x63,
    OPC_JAL = 0x6f,
    OPC_JALR = 0x67,
    OPC_LUI = 0x37,
    OPC_AUIPC = 0x17,
    OPC_SYSTEM = 0x73,
    OPC_MISC_MEM = 0x0f,
};

static u32 u32_get_bits(u32 data, u8 start, u8 end)
{
    assert(start <= end && end < 32);

    const u32 mask = UINT32_MAX >> (31 - (end - start));
    return (data >> start) & mask;
}

struct Instruction decode_instruction(u32 word)
{
    const u32 opcode = u32_get_bits(word, 0, 6);
    (void)opcode;

    return (struct Instruction){0};
}
