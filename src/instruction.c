#include "instruction.h"

#include <assert.h>

static u32 u32_get_gits(u32 data, u8 start, u8 end)
{
    assert(start <= end && end < 32);

    const u32 mask = UINT32_MAX >> (31 - (end - start));
    return (data >> start) & mask;
}

static struct Instruction make_store(u8 opcode,
                                     u8 rs1,
                                     u8 rs2,
                                     u8 funct3,
                                     i32 imm)
{
    const struct StoreInstruction inner = (struct StoreInstruction){
        .rs1 = rs1,
        .rs2 = rs2,
        .funct3 = funct3,
        .imm = imm,
    };

    return (struct Instruction){
        .kind = IK_STORE,
        .opcode = opcode,
        .as = {inner},
    };
}

struct Instruction decode_instruction(u32 raw_instruction);
