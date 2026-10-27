#include "instruction.h"

#include <assert.h>
#include <stdbool.h>

#include "bit_utils.h"
#include "debug.h"

static i32 sign_extend(u32 value, u8 width)
{
    assert(width >= 1 && width <= 32);

    // relies on gcc/clang: u32 -> i32 keeps the bit pattern and >> on a
    // negative i32 is arithmetic (both implementation-defined in ISO C)
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
    FMT_CSR,
};

struct RawPayload {
    u8 rd, rs1, rs2;
    u8 funct3, funct7;
    i32 imm;
    u16 csr;
};

static struct RawPayload read_payload(u32 word, enum Format fmt)
{
    struct RawPayload out = {0};

    switch (fmt) {
        case FMT_R:
            out.rd = (u8)u32_get_bits(word, 7, 11);
            out.funct3 = (u8)u32_get_bits(word, 12, 14);
            out.rs1 = (u8)u32_get_bits(word, 15, 19);
            out.rs2 = (u8)u32_get_bits(word, 20, 24);
            out.funct7 = (u8)u32_get_bits(word, 25, 31);
            break;

        case FMT_I:
            out.rd = (u8)u32_get_bits(word, 7, 11);
            out.funct3 = (u8)u32_get_bits(word, 12, 14);
            out.rs1 = (u8)u32_get_bits(word, 15, 19);
            out.imm = sign_extend(u32_get_bits(word, 20, 31), 12);
            break;

        case FMT_S:
            out.funct3 = (u8)u32_get_bits(word, 12, 14);
            out.rs1 = (u8)u32_get_bits(word, 15, 19);
            out.rs2 = (u8)u32_get_bits(word, 20, 24);
            out.imm = sign_extend(
                (u32_get_bits(word, 25, 31) << 5) | u32_get_bits(word, 7, 11),
                12);
            break;

        case FMT_B:
            out.funct3 = (u8)u32_get_bits(word, 12, 14);
            out.rs1 = (u8)u32_get_bits(word, 15, 19);
            out.rs2 = (u8)u32_get_bits(word, 20, 24);
            out.imm = sign_extend((u32_get_bits(word, 31, 31) << 12) |
                                      (u32_get_bits(word, 7, 7) << 11) |
                                      (u32_get_bits(word, 25, 30) << 5) |
                                      (u32_get_bits(word, 8, 11) << 1),
                                  13);
            break;

        case FMT_U:
            out.rd = (u8)u32_get_bits(word, 7, 11);
            out.imm =
                (i32)(word & 0xFFFFF000u);  // already shifted, low 12 bits zero
            break;

        case FMT_J:
            out.rd = (u8)u32_get_bits(word, 7, 11);
            out.imm = sign_extend((u32_get_bits(word, 31, 31) << 20) |
                                      (u32_get_bits(word, 12, 19) << 12) |
                                      (u32_get_bits(word, 20, 20) << 11) |
                                      (u32_get_bits(word, 21, 30) << 1),
                                  21);
            break;

        case FMT_CSR:
            out.rd = (u8)u32_get_bits(word, 7, 11);
            out.funct3 = (u8)u32_get_bits(word, 12, 14);
            out.rs1 = (u8)u32_get_bits(word, 15, 19);
            out.csr = (u16)u32_get_bits(word, 20, 31);
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

    enum Mnemonic m;

    switch (data.funct3) {
        case 0:
            if (data.funct7 == 0x00) {
                m = OP_ADD;
            } else if (data.funct7 == 0x20) {
                m = OP_SUB;
            } else {
                return DR_BAD_FUNCT7;
            }
            break;
        case 1:
            if (data.funct7 == 0x00) {
                m = OP_SLL;
            } else {
                return DR_BAD_FUNCT7;
            }
            break;
        case 2:
            if (data.funct7 == 0x00) {
                m = OP_SLT;
            } else {
                return DR_BAD_FUNCT7;
            }
            break;
        case 3:
            if (data.funct7 == 0x00) {
                m = OP_SLTU;
            } else {
                return DR_BAD_FUNCT7;
            }
            break;
        case 4:
            if (data.funct7 == 0x00) {
                m = OP_XOR;
            } else {
                return DR_BAD_FUNCT7;
            }
            break;
        case 5:
            if (data.funct7 == 0x00) {
                m = OP_SRL;
            } else if (data.funct7 == 0x20) {
                m = OP_SRA;
            } else {
                return DR_BAD_FUNCT7;
            }
            break;
        case 6:
            if (data.funct7 == 0x00) {
                m = OP_OR;
            } else {
                return DR_BAD_FUNCT7;
            }
            break;
        case 7:
            if (data.funct7 == 0x00) {
                m = OP_AND;
            } else {
                return DR_BAD_FUNCT7;
            }
            break;
        default:
            UNREACHABLE();
    }

    *out = (struct Instruction){.raw = word,
                                .mnemonic = m,
                                .rd = data.rd,
                                .rs1 = data.rs1,
                                .rs2 = data.rs2};
    return DR_OK;
}

static enum DecodeResult decode_op_imm(u32 word,
                                       struct Instruction out[static 1])
{
    const struct RawPayload data = read_payload(word, FMT_I);
    const u32 upper = u32_get_bits(word, 25, 31);  // imm[11:5]
    const u32 shamt = u32_get_bits(word, 20, 24);  // imm[4:0]

    enum Mnemonic m;
    i32 imm = data.imm;

    switch (data.funct3) {
        case 0:
            m = OP_ADDI;
            break;
        case 1:
            if (upper == 0x00) {
                m = OP_SLLI;
                imm = (i32)shamt;
            } else {
                return DR_BAD_FUNCT7;
            }
            break;
        case 2:
            m = OP_SLTI;
            break;
        case 3:
            m = OP_SLTIU;
            break;
        case 4:
            m = OP_XORI;
            break;
        case 5:
            if (upper == 0x00) {
                m = OP_SRLI;
                imm = (i32)shamt;
            } else if (upper == 0x20) {
                m = OP_SRAI;
                imm = (i32)shamt;
            } else {
                return DR_BAD_FUNCT7;
            }
            break;
        case 6:
            m = OP_ORI;
            break;
        case 7:
            m = OP_ANDI;
            break;
        default:
            UNREACHABLE();
    }

    *out = (struct Instruction){
        .raw = word, .mnemonic = m, .rd = data.rd, .rs1 = data.rs1, .imm = imm};
    return DR_OK;
}

static enum DecodeResult decode_load(u32 word, struct Instruction out[static 1])
{
    const struct RawPayload data = read_payload(word, FMT_I);

    enum Mnemonic m;

    switch (data.funct3) {
        case 0:
            m = OP_LB;
            break;
        case 1:
            m = OP_LH;
            break;
        case 2:
            m = OP_LW;
            break;
        case 4:
            m = OP_LBU;
            break;
        case 5:
            m = OP_LHU;
            break;
        default:
            // 3, 6, 7 are LD, LWU and reserved, none of which exist on RV32
            return DR_BAD_FUNCT3;
    }

    *out = (struct Instruction){.raw = word,
                                .mnemonic = m,
                                .rd = data.rd,
                                .rs1 = data.rs1,
                                .imm = data.imm};
    return DR_OK;
}

static enum DecodeResult decode_store(u32 word,
                                      struct Instruction out[static 1])
{
    const struct RawPayload data = read_payload(word, FMT_S);

    enum Mnemonic m;

    switch (data.funct3) {
        case 0:
            m = OP_SB;
            break;
        case 1:
            m = OP_SH;
            break;
        case 2:
            m = OP_SW;
            break;
        default:
            // 3 is SD on RV64, the rest are reserved
            return DR_BAD_FUNCT3;
    }

    *out = (struct Instruction){.raw = word,
                                .mnemonic = m,
                                .rs1 = data.rs1,
                                .rs2 = data.rs2,
                                .imm = data.imm};
    return DR_OK;
}

static enum DecodeResult decode_branch(u32 word,
                                       struct Instruction out[static 1])
{
    const struct RawPayload data = read_payload(word, FMT_B);

    enum Mnemonic m;

    switch (data.funct3) {
        case 0:
            m = OP_BEQ;
            break;
        case 1:
            m = OP_BNE;
            break;
        case 4:
            m = OP_BLT;
            break;
        case 5:
            m = OP_BGE;
            break;
        case 6:
            m = OP_BLTU;
            break;
        case 7:
            m = OP_BGEU;
            break;
        default:
            // 2 and 3 are reserved
            return DR_BAD_FUNCT3;
    }

    *out = (struct Instruction){.raw = word,
                                .mnemonic = m,
                                .rs1 = data.rs1,
                                .rs2 = data.rs2,
                                .imm = data.imm};
    return DR_OK;
}

static enum DecodeResult decode_jal(u32 word, struct Instruction out[static 1])
{
    const struct RawPayload data = read_payload(word, FMT_J);

    *out = (struct Instruction){
        .raw = word, .mnemonic = OP_JAL, .rd = data.rd, .imm = data.imm};
    return DR_OK;
}

static enum DecodeResult decode_jalr(u32 word, struct Instruction out[static 1])
{
    const struct RawPayload data = read_payload(word, FMT_I);

    if (data.funct3 != 0) {
        return DR_BAD_FUNCT3;
    }

    *out = (struct Instruction){.raw = word,
                                .mnemonic = OP_JALR,
                                .rd = data.rd,
                                .rs1 = data.rs1,
                                .imm = data.imm};
    return DR_OK;
}

static enum DecodeResult decode_lui(u32 word, struct Instruction out[static 1])
{
    const struct RawPayload data = read_payload(word, FMT_U);

    *out = (struct Instruction){
        .raw = word, .mnemonic = OP_LUI, .rd = data.rd, .imm = data.imm};
    return DR_OK;
}

static enum DecodeResult decode_auipc(u32 word,
                                      struct Instruction out[static 1])
{
    const struct RawPayload data = read_payload(word, FMT_U);

    *out = (struct Instruction){
        .raw = word, .mnemonic = OP_AUIPC, .rd = data.rd, .imm = data.imm};
    return DR_OK;
}

static enum DecodeResult decode_system(u32 word,
                                       struct Instruction out[static 1])
{
    const struct RawPayload data = read_payload(word, FMT_CSR);

    enum Mnemonic m;

    switch (data.funct3) {
        case 0:
            // SFENCE.VMA is the one operand-carrying instruction here
            if (u32_get_bits(word, 25, 31) == 0x09 && data.rd == 0) {
                *out = (struct Instruction){
                    .raw = word,
                    .mnemonic = OP_SFENCE_VMA,
                    .rs1 = data.rs1,
                    .rs2 = (u8)u32_get_bits(word, 20, 24),
                };
                return DR_OK;
            }

            // the others have no operands: each is identified by its whole
            // word
            switch (word) {
                case 0x00000073:
                    *out =
                        (struct Instruction){.raw = word, .mnemonic = OP_ECALL};
                    return DR_OK;
                case 0x00100073:
                    *out = (struct Instruction){.raw = word,
                                                .mnemonic = OP_EBREAK};
                    return DR_OK;
                case 0x30200073:
                    *out =
                        (struct Instruction){.raw = word, .mnemonic = OP_MRET};
                    return DR_OK;
                case 0x10200073:
                    *out =
                        (struct Instruction){.raw = word, .mnemonic = OP_SRET};
                    return DR_OK;
                case 0x70200073:
                    *out =
                        (struct Instruction){.raw = word, .mnemonic = OP_MNRET};
                    return DR_OK;
                case 0x7b200073:
                    *out =
                        (struct Instruction){.raw = word, .mnemonic = OP_DRET};
                    return DR_OK;
                case 0x10500073:
                    *out =
                        (struct Instruction){.raw = word, .mnemonic = OP_WFI};
                    return DR_OK;
                default:
                    // hypervisor fences, SINVAL.VMA, ... are legal RISC-V,
                    // just not decoded
                    return DR_UNSUPPORTED;
            }
        case 1:
            m = OP_CSRRW;
            break;
        case 2:
            m = OP_CSRRS;
            break;
        case 3:
            m = OP_CSRRC;
            break;
        case 4:
            // the hypervisor extension's loads/stores (HLV, HSV)
            return DR_UNSUPPORTED;
        case 5:
            m = OP_CSRRWI;
            break;
        case 6:
            m = OP_CSRRSI;
            break;
        case 7:
            m = OP_CSRRCI;
            break;
        default:
            UNREACHABLE();
    }

    *out = (struct Instruction){
        .raw = word, .mnemonic = m, .rd = data.rd, .csr = data.csr};

    // funct3 bit 2 selects the immediate forms, where bits 19:15 are uimm
    if (data.funct3 & 0x4) {
        out->imm = data.rs1;
    } else {
        out->rs1 = data.rs1;
    }

    return DR_OK;
}

static enum DecodeResult decode_misc_mem(u32 word,
                                         struct Instruction out[static 1])
{
    const struct RawPayload data = read_payload(word, FMT_I);

    switch (data.funct3) {
        case 0:
            // rd and rs1 are reserved and ignored. Every fm value decodes:
            // 1000 is FENCE.TSO, the others are reserved and behave as a
            // normal FENCE.
            *out = (struct Instruction){
                .raw = word,
                .mnemonic = OP_FENCE,
                .rd = data.rd,
                .rs1 = data.rs1,
                .imm = (i32)u32_get_bits(word, 20, 31),
            };
            return DR_OK;
        case 1:  // FENCE.I (Zifencei)
            *out = (struct Instruction){
                .raw = word,
                .mnemonic = OP_FENCE_I,
                .rd = data.rd,
                .rs1 = data.rs1,
                .imm = data.imm,
            };
            return DR_OK;
        default:
            return DR_BAD_FUNCT3;
    }
}

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
            return decode_op_imm(word, out);
        case OPC_LOAD:
            return decode_load(word, out);
        case OPC_STORE:
            return decode_store(word, out);
        case OPC_BRANCH:
            return decode_branch(word, out);
        case OPC_JAL:
            return decode_jal(word, out);
        case OPC_JALR:
            return decode_jalr(word, out);
        case OPC_LUI:
            return decode_lui(word, out);
        case OPC_AUIPC:
            return decode_auipc(word, out);
        case OPC_SYSTEM:
            return decode_system(word, out);
        case OPC_MISC_MEM:
            return decode_misc_mem(word, out);
        default:
            return DR_BAD_OPCODE;
    }
}
