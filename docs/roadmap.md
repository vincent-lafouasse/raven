# Roadmap

Scope: RV32 only. The C extension is planned. The long-term goal is to boot a
32-bit port of xv6 (Sv32 paging) on a QEMU `virt`-like machine.

| # | Milestone                         | Status      |
|---|-----------------------------------|-------------|
| 1 | RV32I, passing `rv32ui`           | done        |
| 2 | Machine mode: Zicsr and traps     | next        |
| 3 | Interrupts: CLINT timer, `wfi`    | later       |
| 4 | M, A, C extensions                | later       |
| 5 | Supervisor/user mode, Sv32, PLIC, UART, virtio: xv6 | later |

## Milestone 2: machine mode

The machine a typical RISC-V microcontroller implements: one hart, machine
mode only (no supervisor or user mode, no paging), with the Zicsr extension
and synchronous exceptions taken architecturally.

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
| `mtvec`                            | base address, mode forced to direct                       |
| `mepc`                             | read/write; bit 0 reads 0 (bit 1 too while C is off)      |
| `mcause`                           | written on trap entry, values from `src/trap.h`           |
| `mtval`                            | faulting address or instruction bits                      |
| `mscratch`                         | plain read/write                                          |
| `misa`                             | read-only: MXL = 1 (32-bit) plus implemented extensions   |
| `mhartid`                          | read-only 0                                               |
| `mvendorid`, `marchid`, `mimpid`   | read-only 0                                               |
| `mie`, `mip`                       | read as 0 until milestone 3                               |
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
4. `pc = mtvec` base

The trapping instruction has no other effect: no register or memory write.

**`mret`**: `pc = mepc`, `mstatus.MIE = mstatus.MPIE`, `mstatus.MPIE = 1`.
With machine mode only there is no privilege change.

### Out of scope

Supervisor and user modes, delegation (`medeleg`, `mideleg`), paging,
PMP, interrupts, debug mode and triggers.

### Open question: who handles `ecall`

Once traps are architectural, `ecall` jumps to the guest's handler at
`mtvec` and raven's syscall table no longer applies. Options:

1. Keep raven handling `ecall` (current behaviour) as an environment mode for
   simple test programs, and take real traps when the program installs a
   handler or with a `--machine` flag.
2. Real traps only. The guest stops the machine through a device: QEMU
   `virt`'s test finisher at `0x100000`, or the `tohost` symbol that
   riscv-tests' upstream environment writes to.

### Done when

- The `rv32mi` riscv-tests pass under the **upstream** `env/p` environment,
  unmodified (skipping tests that need PMP or debug triggers).
- `test/env/riscv_test.h` is no longer needed for `rv32ui` either.
