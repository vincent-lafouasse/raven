# Roadmap

Scope: RV32 only. The C extension is planned. The long-term goal is to boot a
32-bit port of xv6 (Sv32 paging) on a QEMU `virt`-like machine.

| # | Milestone                         | Status      |
|---|-----------------------------------|-------------|
| 1 | RV32I, passing `rv32ui`           | done        |
| 2 | Machine mode: Zicsr and traps     | in progress |
| 3 | Interrupts: CLINT timer, `wfi`    | later       |
| 4 | M, A, C extensions                | later       |
| 5 | Supervisor/user mode, Sv32, PLIC, UART, virtio: xv6 | later |

## Milestone 2: machine mode

The machine a typical RISC-V microcontroller implements: one hart, machine
mode only (no supervisor or user mode, no paging), with the Zicsr extension
and synchronous exceptions taken architecturally.

### Progress

All 40 `rv32ui` tests pass under riscv-tests' **upstream** `env/p`
environment, unmodified. That exercises the whole trap path: the env's
reset code probes optional CSRs (`mnstatus`, `satp`, PMP, delegation) by
letting them trap, installs its handler in `mtvec`, enters the test body
with `mret`, and ends every test with an `ecall` its handler turns into a
`tohost` write.

Done:

- Zicsr: all six instructions, with the read/write side-effect rules.
- The CSRs in the table below, except the counters.
- Trap entry and `mret`.
- Diagnostics: every trap carries a non-architectural detail (why an
  instruction was illegal, what the bus reported) and is logged when taken.
- `run_riscv_test`, a test bench around `raven_core` that watches `tohost`
  and turns the env's result into an exit code.

Left:

- `rv32mi` under the upstream env.
- `wfi` (decoded, still raises an illegal instruction).
- The counters (`mcycle`, `minstret`, ...), if `rv32mi` needs them.

### In scope

**Instructions**

- Zicsr: `csrrw`, `csrrs`, `csrrc`, `csrrwi`, `csrrsi`, `csrrci`
  (SYSTEM opcode, funct3 != 0).
  - `csrrw`/`csrrwi` with `rd = x0` do not read the CSR.
  - `csrrs`/`csrrc` with `rs1 = x0`, and `csrrsi`/`csrrci` with a zero
    immediate, do not write it.
- `mret`: return from a trap.
- `wfi`: a no-op until interrupts exist (allowed by the spec).
- `ecall` and `ebreak` raise exceptions (causes 11 and 3).

**CSRs**

| CSR                                | Behaviour in raven                                        |
|------------------------------------|-----------------------------------------------------------|
| `mstatus`                          | only MIE, MPIE, MPP; MPP hardwired to M (`11`), the rest reads 0 |
| `mstatush`                         | reads 0, writes ignored                                   |
| `mtvec`                            | base address, mode forced to direct                       |
| `mepc`                             | read/write; bits 1:0 read 0 while C is off                |
| `mcause`                           | written on trap entry, values from `src/trap_cause.h`     |
| `mtval`                            | faulting address or instruction bits                      |
| `mscratch`                         | plain read/write                                          |
| `misa`                             | read-only: MXL = 1 (32-bit) plus implemented extensions   |
| `mhartid`                          | read-only 0                                               |
| `mvendorid`, `marchid`, `mimpid`   | read-only 0                                               |
| `mie`                              | stores MSIE, MTIE, MEIE; no effect until milestone 3      |
| `mip`                              | reads 0, writes ignored, until milestone 3                |
| `mcycle`, `minstret`, `cycle`, `instret` (+ `h` halves) | optional counters                    |

**CSR access rules**

- Accessing a CSR that does not exist raises an illegal instruction.
- Writing a read-only CSR (address bits 11:10 = `11`) raises an illegal
  instruction.
- Writes to WARL fields keep only the bits raven supports.

**Trap entry** (exceptions only)

1. `mepc = pc` of the trapping instruction
2. `mcause = cause`, `mtval = tval`
3. `mstatus.MPIE = mstatus.MIE`, `mstatus.MIE = 0`
4. `mstatus.MPP` = the current mode, then the mode becomes M
5. `pc = mtvec` base

The trapping instruction has no other effect: no register or memory write.

**`mret`**: `pc = mepc`, mode = `mstatus.MPP`, `mstatus.MIE = mstatus.MPIE`,
`mstatus.MPIE = 1`, `mstatus.MPP` = the least privileged supported mode.
With machine mode only, MPP is always M, so there is no privilege change.
Outside M-mode, `mret` raises an illegal instruction.

### Out of scope

Supervisor and user modes, delegation (`medeleg`, `mideleg`), paging,
PMP, interrupts, debug mode and triggers.

### Decided: who handles `ecall`

Guest software, like any other trap. Raven takes every trap
architecturally and never interprets `ecall`: system calls are an ABI
between guest programs and a guest kernel. The old syscall shortcut and
raven's own test environment are gone from the emulator.

A guest reports results to the outside world through a device or a
memory location a test bench watches, never through raven itself. For
riscv-tests that is `tohost`, watched by `run_riscv_test` (outside
`raven_core`). For `virt`-style guests it will be the test finisher at
`0x100000`.

### Done when

- The `rv32mi` riscv-tests pass under the **upstream** `env/p` environment,
  unmodified (skipping tests that need PMP or debug triggers).
- ~~`test/riscv-tests/env/riscv_test.h` is no longer needed for `rv32ui`
  either.~~ Done: `rv32ui` runs under the upstream env.
