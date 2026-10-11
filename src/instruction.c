#include "instruction.h"

#include <assert.h>
#include <stdbool.h>

#include "utils.h"

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

enum Format {
    FMT_R,
    FMT_I,
    FMT_S,
    FMT_U,
    FMT_J,
    FMT_B,
};

struct RawPayload {
    u8 rd, rs1, rs2;
    u8 funct3, funct7;
    i32 imm;
};

static struct RawPayload read_payload(u32 word, enum Format fmt)
{
    struct RawPayload out = {0};

    switch (fmt) {
        case FMT_R:
            out.rd = u32_get_bits(word, 7, 11);
            out.funct3 = u32_get_bits(word, 12, 14);
            out.rs1 = u32_get_bits(word, 15, 19);
            out.rs2 = u32_get_bits(word, 20, 24);
            out.funct7 = u32_get_bits(word, 25, 31);
            break;

        case FMT_I:
            out.rd = u32_get_bits(word, 7, 11);
            out.funct3 = u32_get_bits(word, 12, 14);
            out.rs1 = u32_get_bits(word, 15, 19);
            out.imm = sign_extend(u32_get_bits(word, 20, 31), 12);
            break;

        case FMT_S:
            out.funct3 = u32_get_bits(word, 12, 14);
            out.rs1 = u32_get_bits(word, 15, 19);
            out.rs2 = u32_get_bits(word, 20, 24);
            out.imm = sign_extend(
                (u32_get_bits(word, 25, 31) << 5) | u32_get_bits(word, 7, 11),
                12);
            break;

        case FMT_B:
            out.funct3 = u32_get_bits(word, 12, 14);
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

static enum DecodeResult decode_op(u32 word, struct Instruction out[static 1])
{
    const struct RawPayload data = read_payload(word, FMT_R);
    out->rd = data.rd;
    out->rs1 = data.rs1;
    out->rs2 = data.rs2;

    // only error path is BAD_FUNCT7
    bool ok = true;

    switch (data.funct3) {
        case 0:
            if (data.funct7 == 0x00) {
                out->mnemonic = OP_ADD;
            } else if (data.funct7 == 0x20) {
                out->mnemonic = OP_SUB;
            } else {
                ok = false;
            }
        case 1:
            if (data.funct7 == 0x00) {
                out->mnemonic = OP_SLL;
            } else {
                ok = false;
            }
        case 2:
            if (data.funct7 == 0x00) {
                out->mnemonic = OP_SLT;
            } else {
                ok = false;
            }
        case 3:
            if (data.funct7 == 0x00) {
                out->mnemonic = OP_SLTU;
            } else {
                ok = false;
            }
        default:
            UNREACHABLE();
    }

    if (ok)
        return DR_OK;
    else
        return DR_BAD_FUNCT7;
}

enum DecodeResult decode_op_imm(u32 word, struct Instruction out[static 1]);
enum DecodeResult decode_load(u32 word, struct Instruction out[static 1]);
enum DecodeResult decode_store(u32 word, struct Instruction out[static 1]);
enum DecodeResult decode_branch(u32 word, struct Instruction out[static 1]);
enum DecodeResult decode_jal(u32 word, struct Instruction out[static 1]);
enum DecodeResult decode_jalr(u32 word, struct Instruction out[static 1]);
enum DecodeResult decode_lui(u32 word, struct Instruction out[static 1]);
enum DecodeResult decode_auipc(u32 word, struct Instruction out[static 1]);
enum DecodeResult decode_system(u32 word, struct Instruction out[static 1]);
enum DecodeResult decode_misc_mem(u32 word, struct Instruction out[static 1]);

enum DecodeResult decode_instruction(u32 word, struct Instruction out[static 1])
{
    if ((word & 0x3) != 0x3) {
        return DR_BAD_LENGTH;
    }

    const u32 opcode = u32_get_bits(word, 0, 6);
    switch (opcode) {
        case OPC_OP:
            return decode_op(word, out);
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
            return DR_BAD_OPCODE;
    }
}
