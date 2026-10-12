#pragma once

#include "bus.h"

enum LoadStatus {
    LOAD_OK = 0,
    LOAD_CANNOT_OPEN,
    LOAD_CANNOT_READ,
    LOAD_TOO_BIG,  // does not fit in RAM
    LOAD_UNSUPPORTED,
};

// Copies the whole file to RAM_BASE. The entry point is RAM_BASE.
enum LoadStatus load_flat(struct Bus bus[static 1],
                          const char* path,
                          u32 entry[static 1]);

// Loads the PT_LOAD segments of a static RV32 ELF executable.
// The entry point is e_entry.
enum LoadStatus load_elf(struct Bus bus[static 1],
                         const char* path,
                         u32 entry[static 1]);
