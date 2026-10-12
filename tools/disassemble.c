#include <stdio.h>

#include "instruction.h"

struct Io {
    FILE* in;
    FILE* out;
};

struct Io io_open_or_die(const char* infile)
{
    (void)infile;
    return (struct Io){0};
}

void io_close(struct Io io[static 1])
{
    fclose(io->in);
    fclose(io->out);
}

int main(int ac, char** av)
{
    if (ac != 2) {
        fprintf(stderr, "Usage: %s program.bin\n", av[0]);
        return 1;
    }

    struct Io io = io_open_or_die(av[1]);

    io_close(&io);
    return 0;
}
