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
