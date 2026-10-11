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

enum Format {
    FMT_R,
    FMT_I,
    FMT_S,
    FMT_U,
    FMT_J,
    FMT_B,
};

struct Payload {
    u8 rd, rs1, rs2;
    i32 imm;
};

static u32 u32_get_bits(u32 data, u8 start, u8 end)
{
    assert(start <= end && end < 32);

    const u32 mask = UINT32_MAX >> (31 - (end - start));
    return (data >> start) & mask;
}

static i32 sign_extend(u32 value, u8 width)
{
    assert(width >= 1 && width <= 32);

    const u8 shift = 32 - width;
    return (i32)(value << shift) >> shift;
}

static struct Payload read_payload(u32 word, enum Format fmt)
{
    struct Payload out = {0};

    switch (fmt) {
        case FMT_R:
            out.rd = u32_get_bits(word, 7, 11);
            out.rs1 = u32_get_bits(word, 15, 19);
            out.rs2 = u32_get_bits(word, 20, 24);
            break;

        case FMT_I:
            out.rd = u32_get_bits(word, 7, 11);
            out.rs1 = u32_get_bits(word, 15, 19);
            out.imm = sign_extend(u32_get_bits(word, 20, 31), 12);
            break;

        case FMT_S:
            out.rs1 = u32_get_bits(word, 15, 19);
            out.rs2 = u32_get_bits(word, 20, 24);
            out.imm = sign_extend(
                (u32_get_bits(word, 25, 31) << 5) | u32_get_bits(word, 7, 11),
                12);
            break;

        case FMT_B:
            out.rs1 = u32_get_bits(word, 15, 19);
            out.rs2 = u32_get_bits(word, 20, 24);
            out.imm = sign_extend((u32_get_bits(word, 31, 31) << 12) |
                                      (u32_get_bits(word, 7, 7) << 11) |
                                      (u32_get_bits(word, 25, 30) << 5) |
                                      (u32_get_bits(word, 8, 11) << 1),
                                  13);
            break;

        case FMT_U:
            out.rd = u32_get_bits(word, 7, 11);
            out.imm =
                (i32)(word & 0xFFFFF000u);  // already shifted, low 12 bits zero
            break;

        case FMT_J:
            out.rd = u32_get_bits(word, 7, 11);
            out.imm = sign_extend((u32_get_bits(word, 31, 31) << 20) |
                                      (u32_get_bits(word, 12, 19) << 12) |
                                      (u32_get_bits(word, 20, 20) << 11) |
                                      (u32_get_bits(word, 21, 30) << 1),
                                  21);
            break;
    }

    return out;
}

struct Instruction decode_instruction(u32 word)
{
    const u32 opcode = u32_get_bits(word, 0, 6);
    (void)opcode;

    switch (opcode) {
        case OPC_OP:
        case OPC_OP_IMM:
        case OPC_LOAD:
        case OPC_STORE:
        case OPC_BRANCH:
        case OPC_JAL:
        case OPC_JALR:
        case OPC_LUI:
        case OPC_AUIPC:
        case OPC_SYSTEM:
        case OPC_MISC_MEM:
        default:
            break;
    }

    return (struct Instruction){0};
}
