#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitcast.h"
#include "instruction.h"

__attribute__((noreturn, format(printf, 1, 2))) static void die(const char* fmt,
                                                                ...)
{
    fflush(stdout);

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

static struct Buffer load_bytes_or_die(const char* path)
{
    FILE* f = fopen(path, "rb");
    if (!f) {
        die("cannot open %s", path);
    }

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    if (sz < 0) {
        die("cannot read %s", path);
    }
    rewind(f);

    u8* data = malloc(sz > 0 ? (size_t)sz : 1);
    if (!data || fread(data, 1, sz, f) != (size_t)sz) {
        die("cannot read %s", path);
    }

    fclose(f);
    return (struct Buffer){.data = data, .sz = (size_t)sz};
}

int main(int ac, char** av)
{
    if (ac != 2) {
        die("Usage: %s program.bin", av[0]);
    }

    const struct Buffer buf = load_bytes_or_die(av[1]);
    const size_t n_words = buf.sz / 4;

    char line[INSTRUCTION_STR_LEN];
    for (size_t i = 0; i < n_words; ++i) {
        const u32 word = bitcastU32_LE(buf.data + 4 * i);

        struct Instruction instr;
        if (decode_instruction(word, &instr) == DR_OK) {
            format_instruction(instr, line);
            printf("    %s\n", line);
        } else {
            // keep the output reassemblable, byte for byte
            printf("    .word 0x%08x\n", word);
        }
    }

    // trailing bytes that don't make a full word
    for (size_t i = 4 * n_words; i < buf.sz; ++i) {
        printf("    .byte 0x%02x\n", buf.data[i]);
    }

    free(buf.data);
    return 0;
}
