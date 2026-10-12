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

struct Io {
    FILE* in;
    FILE* out;
};

struct Io io_open_or_die(const char* infile)
{
    const char* suffix = ".s";
    const size_t outfile_path_len = strlen(infile) + strlen(suffix);
    char* outfile_path = calloc(1, outfile_path_len + 1);
    if (outfile_path == NULL) {
        die("OOM");
    }

    strcat(outfile_path, infile);
    strcat(outfile_path, suffix);

    FILE* in = fopen(infile, "rb");
    if (in == NULL) {
        die("Failed to open file %s", infile);
    }

    FILE* out = fopen(outfile_path, "wb");
    if (out == NULL) {
        die("Failed to open file %s", outfile_path);
    }

    free(outfile_path);
    return (struct Io){.in = in, .out = out};
}

void io_close(struct Io io[static 1])
{
    fclose(io->in);
    fclose(io->out);
}

int main(int ac, char** av)
{
    if (ac != 2) {
        die("Usage: %s program.bin\n", av[0]);
    }

    struct Io io = io_open_or_die(av[1]);

    io_close(&io);
    return 0;
}
