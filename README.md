# raven

An RV32I emulator in C99.

raven emulates a small machine modelled on QEMU's `virt` platform: an RV32I
hart with RAM at the same address and, later, a subset of the same devices.
Bare-metal programs linked for `virt` should run unmodified on raven, which
makes QEMU a reference to compare against.

## Machine

- ISA: RV32I (unprivileged base integer ISA), complete and passing the
  `rv32ui` tests. `fence` is a no-op.
- No CSRs or privilege modes yet. Traps are raised with their RISC-V cause
  codes, but raven handles them itself (see Environment calls) instead of
  delivering them to a guest handler.
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

Any other `a7` is a fatal error. `ebreak` stops the machine with an error.

## Roadmap

RV32 only, C extension planned, aiming at a 32-bit port of xv6. Next
milestone: machine mode with Zicsr and traps. See
[docs/roadmap.md](docs/roadmap.md).

## Tools

- `raven`: the emulator. `raven --flat program.bin` loads a flat binary at
  `0x80000000` and runs it; `--elf` is not implemented yet.
- `raven-dis`: disassembles a flat RV32I binary to GNU `as` syntax. Words
  that do not decode are printed as `.word`, so the output reassembles to the
  same bytes.

## Building

    make            # debug build, in build/debug
    make release    # release build, in build/release

## Tests

Both need a RISC-V cross toolchain (`riscv64-unknown-elf-*`). `make test`
runs both through ctest.

- `test/decoder-test/`: decoder round trip. `make_test.py` generates `test.s`,
  `test.sh` assembles it, runs it through `raven-dis`, reassembles the output
  and compares the binaries.
- `third_party/riscv-tests`: the `rv32ui` tests, vendored from
  riscv-software-src/riscv-tests. They build against raven's own environment
  in `test/env/riscv_test.h` instead of upstream's boot code.
  - `test/compile.sh --flat|--elf test.S` builds one test.
  - `test/run-riscv-tests.sh [name...]` builds and runs them, reporting PASS,
    FAIL (a check in the test failed) or ERROR (build or raven failed).

### rv32ui status

All 40 tests pass: integer computation, upper immediates, branches, jumps,
loads, stores and the mixed `simple`, `ld_st` and `st_ld` tests.

Skipped: `fence_i` (needs Zifencei) and `ma_data` (misaligned accesses,
which raven rejects by design).

The tests are built with `-mno-relax`: linker relaxation would rewrite `la`
into gp-relative addressing, but riscv-tests uses `gp` as the test number.
