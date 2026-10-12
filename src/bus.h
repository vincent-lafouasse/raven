#pragma once

#include "types.h"

// Memory map, a subset of QEMU's `virt` machine (hw/riscv/virt.c, virt_memmap)
//
//   0x80000000 .. 0x800fffff   RAM (1 MiB)
//
// Anything else is unmapped.

#define RAM_BASE 0x80000000u
#define RAM_SIZE (1u << 20)  // 1 MiB

struct Bus {};

enum BusStatus { BUS_OK, BUS_UNMAPPED, BUS_MISALIGNED };

enum BusStatus bus_read(struct Bus bus[static 1],
                        u32 addr,
                        u32 size,
                        u32 out[static 1]);
enum BusStatus bus_write(struct Bus bus[static 1],
                         u32 addr,
                         u32 size,
                         u32 value);
