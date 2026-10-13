# raven

An RV32I emulator in C99.

raven emulates a small machine modelled on QEMU's `virt` platform: an RV32I
hart with RAM at the same address and, later, a subset of the same devices.
Bare-metal programs linked for `virt` should run unmodified on raven, which
makes QEMU a reference to compare against.

## Machine

- ISA: RV32I (unprivileged base integer ISA only). No CSRs, no privilege
  modes, no traps. `fence` is a no-op.
- One hart. All registers start at 0, `pc` starts at the entry point.

### Memory map

| Address      | Size  | Device | Status  |
|--------------|-------|--------|---------|
| `0x80000000` | 1 MiB | RAM    | done    |
| `0x10000000` |       | UART   | later   |

Any access outside a mapped region, or a misaligned access, stops the
machine with an error.

### Environment calls

`ecall` follows the Linux syscall convention: number in `a7`, arguments in
`a0`..`a5`.

| `a7` | Call         |
|------|--------------|
| 93   | `exit(a0)`   |

`ebreak` stops the machine.

## Tools

- `raven`: the emulator (in progress).
- `raven-dis`: disassembles a flat RV32I binary to GNU `as` syntax. Words
  that do not decode are printed as `.word`, so the output reassembles to the
  same bytes.

## Building

    make            # debug build, in build/debug
    make release    # release build, in build/release

## Tests

- `test/`: decoder round trip. `make_test.py` generates `test.s`, `test.sh`
  assembles it, runs it through `raven-dis`, reassembles the output and
  compares the binaries.
- `third_party/riscv-tests`: the `rv32ui` tests, vendored from
  riscv-software-src/riscv-tests. They build against raven's own environment
  in `test/env/riscv_test.h` instead of upstream's boot code.
