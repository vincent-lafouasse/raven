#include "bus.h"

#include <assert.h>
#include <stdbool.h>

static enum BusStatus ram_offset(u32 addr, u32 size, u32 offset[static 1])
{
    assert(size == 1 || size == 2 || size == 4);

    if (addr % size != 0) {
        return BUS_MISALIGNED;
    }
    if (addr < RAM_BASE || addr - RAM_BASE > RAM_SIZE - size) {
        return BUS_UNMAPPED;
    }

    *offset = addr - RAM_BASE;
    return BUS_OK;
}

enum BusStatus bus_read(struct Bus bus[static 1],
                        u32 addr,
                        u32 size,
                        u32 out[static 1])
{
    u32 offset;
    const enum BusStatus status = ram_offset(addr, size, &offset);
    if (status != BUS_OK) {
        return status;
    }

    // little-endian: byte i is bits 8i..8i+7
    u32 value = 0;
    for (u32 i = 0; i < size; ++i) {
        value |= (u32)bus->ram[offset + i] << (8 * i);
    }

    *out = value;
    return BUS_OK;
}

enum BusStatus bus_write(struct Bus bus[static 1],
                         u32 addr,
                         u32 size,
                         u32 value)
{
    u32 offset;
    const enum BusStatus status = ram_offset(addr, size, &offset);
    if (status != BUS_OK) {
        return status;
    }

    // only the low `size` bytes of value are written
    for (u32 i = 0; i < size; ++i) {
        bus->ram[offset + i] = (u8)(value >> (8 * i));
    }

    return BUS_OK;
}
