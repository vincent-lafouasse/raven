#include "bus.h"

#include <stdbool.h>

#include "bitcast.h"
#include "debug.h"

static enum BusStatus ram_offset(u32 addr, u32 size, u32 offset[static 1])
{
    // internal callers only; kept in release builds since size == 0 would
    // divide by zero below
    if (size != 1 && size != 2 && size != 4) {
        UNREACHABLE();
    }

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

    const u8* bytes = bus->ram + offset;
    switch (size) {
        case 1:
            *out = bytes[0];
            break;
        case 2:
            *out = bitcastU16_LE(bytes);
            break;
        case 4:
            *out = bitcastU32_LE(bytes);
            break;
        default:
            UNREACHABLE();
    }

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
