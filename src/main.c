#include <assert.h>
#include <stdlib.h>

#include "bus.h"
#include "cpu.h"

int main(void)
{
    struct Cpu cpu;
    cpu_reset(&cpu, RAM_BASE);

    struct Bus* bus = calloc(1, sizeof(*bus));
    assert(bus != NULL);

    free(bus);
    return 0;
}
