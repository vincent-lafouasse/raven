#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bus.h"
#include "cpu.h"
#include "execute.h"
#include "instruction.h"
#include "loader.h"
#include "utils.h"

#define USAGE                           \
    "usage: raven --flat program.bin\n" \
    "       raven --elf program.elf"

__attribute__((noreturn)) static void die(const char* fmt, ...)
{
    fflush(stdout);

    va_list ap;
    va_start(ap, fmt);
    fputs("raven: ", stderr);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);
    exit(EXIT_FAILURE);
}

void trap(bool cond)
{
    if (!cond) {
        TRACE_TRAP();
    }
}

static const char* load_status_str(enum LoadStatus s)
{
    switch (s) {
        case LOAD_OK:
            return "ok";
        case LOAD_CANNOT_OPEN:
            return "cannot open file";
        case LOAD_CANNOT_READ:
            return "cannot read file";
        case LOAD_TOO_BIG:
            return "program does not fit in RAM";
        case LOAD_UNSUPPORTED:
            return "unsupported format";
    }
    return "unknown error";
}

int main(int ac, char** av)
{
    if (ac != 3) {
        die(USAGE);
    }

    const char* const flag = av[1];
    const char* const binary_path = av[2];

    enum LoadStatus (*load)(struct Bus*, const char*, u32*);
    if (strcmp(flag, "--flat") == 0) {
        load = load_flat;
    } else if (strcmp(flag, "--elf") == 0) {
        load = load_elf;
    } else {
        die("unknown option %s\n" USAGE, flag);
    }

    struct Bus* bus = calloc(1, sizeof(*bus));
    if (!bus) {
        die("out of memory");
    }

    u32 entry;
    const enum LoadStatus status = load(bus, binary_path, &entry);
    if (status != LOAD_OK) {
        die("%s: %s", binary_path, load_status_str(status));
    }

    struct Cpu cpu;
    cpu_reset(&cpu, entry);

    char cpu_str[1024] = {0};
    cpu_dump(&cpu, cpu_str);
    printf("%s\n", cpu_str);

    char instruction_str[128] = {0};

    while (1) {
        u32 raw_instruction;
        trap(bus_read(bus, cpu.pc, 4, &raw_instruction) == BUS_OK);

        struct Instruction instruction;
        trap(decode_instruction(raw_instruction, &instruction) == DR_OK);

        format_instruction(instruction, instruction_str);
        printf("%s\n", instruction_str);

        switch (execute(&cpu, bus, instruction)) {
            case EXEC_UNIMPLEMENTED:
                die("Unimplemented instruction");
            case EXEC_UNSUPPORTED:
                die("Unsupported instruction");
            case EXEC_EBREAK:
                // TODO: is this actually ebreak ?
                printf("ebreak\n");
                exit(0);
            case EXEC_BUS_FAULT:
            case EXEC_MISALIGNED_PC:
                // TODO: be less coarse
                die("fatal");
            case EXEC_ECALL:
                // TODO: actually do something with syscalls
                die("ecall");
            case EXEC_OK:
                break;
        }

        cpu_dump(&cpu, cpu_str);
        printf("%s\n", cpu_str);
    }

    free(bus);
    return EXIT_SUCCESS;
}
