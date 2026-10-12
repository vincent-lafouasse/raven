#include "loader.h"

#include <stdbool.h>
#include <stdio.h>

enum LoadStatus load_flat(struct Bus bus[static 1],
                          const char* path,
                          u32 entry[static 1])
{
    FILE* f = fopen(path, "rb");
    if (!f) {
        return LOAD_CANNOT_OPEN;
    }

    const size_t n = fread(bus->ram, 1, RAM_SIZE, f);
    const bool read_error = ferror(f);
    // anything left after RAM_SIZE bytes means the program doesn't fit
    const bool too_big = n == RAM_SIZE && fgetc(f) != EOF;
    fclose(f);

    if (read_error) {
        return LOAD_CANNOT_READ;
    }
    if (too_big) {
        return LOAD_TOO_BIG;
    }

    *entry = RAM_BASE;
    return LOAD_OK;
}

enum LoadStatus load_elf(struct Bus bus[static 1],
                         const char* path,
                         u32 entry[static 1])
{
    // TODO
    (void)bus;
    (void)path;
    (void)entry;
    return LOAD_UNSUPPORTED;
}
