#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "instruction.h"

static void die(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fputs("raven-dis: ", stderr);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);
    exit(EXIT_FAILURE);
}

struct Buffer {
    u8* data;
    size_t sz;
};

struct Buffer load_bytes_or_die(const char* path)
{
    FILE* f = fopen(path, "rb");
    if (!f) {
        die("cannot open %s", path);
    }

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);

    u8* data = malloc(sz);
    if (!data || fread(data, 1, sz, f) != (size_t)sz) {
        die("cannot read %s", path);
    }

    fclose(f);
    return (struct Buffer){.data = data, .sz = (size_t)sz};
}

u32 read_u32_LE(const u8* data)
{
    return (u32)data[0] | (u32)data[1] << 8 | (u32)data[2] << 16 |
           (u32)data[3] << 24;
}

int main(int ac, char** av)
{
    if (ac != 2) {
        die("Usage: %s program.bin\n", av[0]);
    }

    struct Buffer data = load_bytes_or_die(av[1]);

    free(data.data);
    return 0;
}
