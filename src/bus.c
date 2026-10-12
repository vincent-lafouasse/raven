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
    (void)bus;
    (void)addr;
    (void)size;
    (void)out;
    return BUS_OK;
}

enum BusStatus bus_write(struct Bus bus[static 1],
                         u32 addr,
                         u32 size,
                         u32 value)
{
    (void)bus;
    (void)addr;
    (void)size;
    (void)value;
    return BUS_OK;
}
