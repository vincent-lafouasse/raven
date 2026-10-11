#include <string.h>
#include <stdio.h>

#include "instruction.h"

#define USAGE "raven --dump program dump_file"

int main(int ac, char** av)
{
    if (ac != 4 || strcmp(av[1], "--dump") != 0) {
        fprintf(stderr, "Usage: %s\n", USAGE);
        return 1;
    }

    return 0;
}
