#pragma once

#include "types.h"

// TODO: document the memory map

enum BusStatus { BUS_OK, BUS_UNMAPPED, BUS_MISALIGNED };

enum BusStatus bus_read(struct Bus bus[static 1],
                        u32 addr,
                        u32 size,
                        u32 out[static 1]);
enum BusStatus bus_write(struct Bus bus[static 1],
                         u32 addr,
                         u32 size,
                         u32 value);
