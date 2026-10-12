#include "bus.h"

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
