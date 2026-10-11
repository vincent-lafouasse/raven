#!/usr/bin/env python3
"""Generate test.s: RV32I assembly built to stress a decoder.

This script only produces inputs. The expected encodings come from the
assembler (riscv32-unknown-elf-as), which is the external reference.

usage: gen_test.py [-o test.s] [--pad BYTES] [--with-unsupported]
"""
import argparse

# Register numbers used to probe each register field: x0, one-hot, all ones.
REGS = [0, 1, 2, 4, 8, 16, 31]

# Background operands. They have mixed bit patterns and differ from every
# value in REGS, so a field read from the wrong bits changes the output.
RD_BG, RS1_BG, RS2_BG = 5, 21, 10
IMM_BG = 1365  # 0x555
SHAMT_BG = 21
PC_BG_B = 1366  # 0x556, even
PC_BG_J = 349524  # 0x55554, even

R_OPS = ["add", "sub", "sll", "slt", "sltu", "xor", "srl", "sra", "or", "and"]
I_OPS = ["addi", "slti", "sltiu", "xori", "ori", "andi"]
SHIFT_OPS = ["slli", "srli", "srai"]
LOAD_OPS = ["lb", "lh", "lw", "lbu", "lhu"]
STORE_OPS = ["sb", "sh", "sw"]
BRANCH_OPS = ["beq", "bne", "blt", "bge", "bltu", "bgeu"]


def uniq(xs):
    return list(dict.fromkeys(xs))


def imm12():
    """Signed 12-bit values: one-hot, all-ones-but-one, and the extremes."""
    vals = [0, -1, 2047, -2048]
    for k in range(11):  # bits 0..10 (bit 11 is the sign bit, covered above)
        vals += [1 << k, ~(1 << k)]
    return uniq(vals)


def pc_offsets(top):
    """Even pc-relative offsets for a field whose sign bit is bit `top`.

    B-type has top=12 (s13), J-type has top=20 (s21). Bit 0 is implicit.
    Includes positive one-hot values, negative "staircases" (all bits from
    k up to the sign bit set), their single-bit complements and the extremes.
    """
    vals = [0, (1 << top) - 2]
    vals += [1 << k for k in range(1, top)]
    vals += [-(1 << k) for k in range(1, top + 1)]
    vals += [-2 - (1 << k) for k in range(1, top)]
    return uniq(vals)


def imm20():
    """Unsigned 20-bit values for LUI and AUIPC."""
    vals = [0, 0xFFFFF, 0x80000, 0x7FFFF]
    for k in range(20):
        vals += [1 << k, 0xFFFFF ^ (1 << k)]
    return uniq(vals)


def rel(off):
    return f".{off:+d}"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("-o", "--output", default="test.s")
    ap.add_argument(
        "--pad",
        type=int,
        default=1 << 20,
        help="bytes of nop padding before and after the tests "
        "(J-type reaches +-1 MiB, so keep the default for full coverage)",
    )
    ap.add_argument(
        "--with-unsupported",
        action="store_true",
        help="also emit FENCE, which a minimal decoder may reject",
    )
    args = ap.parse_args()

    L = []

    def section(name):
        L.append(f"\n# --- {name} ---")

    def emit(s):
        L.append("    " + s)

    section("R-type")
    for op in R_OPS:
        for r in REGS:
            emit(f"{op} x{r}, x{RS1_BG}, x{RS2_BG}")
            emit(f"{op} x{RD_BG}, x{r}, x{RS2_BG}")
            emit(f"{op} x{RD_BG}, x{RS1_BG}, x{r}")

    section("OP-IMM")
    for op in I_OPS:
        for imm in imm12():
            emit(f"{op} x{RD_BG}, x{RS1_BG}, {imm}")
        for r in REGS:
            emit(f"{op} x{r}, x{RS1_BG}, {IMM_BG}")
            emit(f"{op} x{RD_BG}, x{r}, {IMM_BG}")

    section("shift immediates (every shamt)")
    for op in SHIFT_OPS:
        for sh in range(32):
            emit(f"{op} x{RD_BG}, x{RS1_BG}, {sh}")
        for r in REGS:
            emit(f"{op} x{r}, x{RS1_BG}, {SHAMT_BG}")
            emit(f"{op} x{RD_BG}, x{r}, {SHAMT_BG}")

    section("loads")
    for op in LOAD_OPS:
        for imm in imm12():
            emit(f"{op} x{RD_BG}, {imm}(x{RS1_BG})")
        for r in REGS:
            emit(f"{op} x{r}, {IMM_BG}(x{RS1_BG})")
            emit(f"{op} x{RD_BG}, {IMM_BG}(x{r})")

    section("stores")
    for op in STORE_OPS:
        for imm in imm12():
            emit(f"{op} x{RS2_BG}, {imm}(x{RS1_BG})")
        for r in REGS:
            emit(f"{op} x{r}, {IMM_BG}(x{RS1_BG})")
            emit(f"{op} x{RS2_BG}, {IMM_BG}(x{r})")

    section("branches")
    for op in BRANCH_OPS:
        for off in pc_offsets(12):
            emit(f"{op} x{RS1_BG}, x{RS2_BG}, {rel(off)}")
        for r in REGS:
            emit(f"{op} x{r}, x{RS2_BG}, {rel(PC_BG_B)}")
            emit(f"{op} x{RS1_BG}, x{r}, {rel(PC_BG_B)}")

    section("jal")
    for off in pc_offsets(20):
        emit(f"jal x{RD_BG}, {rel(off)}")
    for r in REGS:
        emit(f"jal x{r}, {rel(PC_BG_J)}")

    section("jalr")
    for imm in imm12():
        emit(f"jalr x{RD_BG}, {imm}(x{RS1_BG})")
    for r in REGS:
        emit(f"jalr x{r}, {IMM_BG}(x{RS1_BG})")
        emit(f"jalr x{RD_BG}, {IMM_BG}(x{r})")

    section("lui / auipc")
    for op in ["lui", "auipc"]:
        for imm in imm20():
            emit(f"{op} x{RD_BG}, 0x{imm:x}")
        for r in REGS:
            emit(f"{op} x{r}, 0x55555")

    section("system")
    emit("ecall")
    emit("ebreak")

    if args.with_unsupported:
        section("misc-mem (often rejected by a minimal decoder)")
        emit("fence")
        emit("fence rw, rw")

    nops = args.pad // 4
    pad = f"    .fill {nops}, 4, 0x00000013    # nops"
    header = (
        "# generated by gen_test.py, do not edit\n"
        "    .option norelax\n"
        "    .option norvc\n"
        "    .text\n"
        "    .balign 4\n"
        "\n# padding so pc-relative targets stay inside the section"
    )
    text = "\n".join([header, pad, *L, "\n# padding", pad]) + "\n"

    with open(args.output, "w") as f:
        f.write(text)

    n = sum(1 for line in L if line.startswith("    "))
    print(f"wrote {args.output}: {n} test instructions, {nops} nops of padding each side")


if __name__ == "__main__":
    main()
