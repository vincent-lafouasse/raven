#!/bin/sh
# Builds one riscv-tests source (or any RV32 .S) for raven.
#
#   test/riscv-tests/compile.sh [--env upstream|raven] --elf  path/to/test.S [out.elf]
#   test/riscv-tests/compile.sh [--env upstream|raven] --flat path/to/test.S [out.bin]
#
# Output defaults to build/riscv-tests/<env>/<name>.elf or .bin at the repo
# root. A flat binary is the linked ELF flattened with objcopy, so _start ends
# up as its first byte.
#
# Environments (the riscv_test.h the test is built against):
#   upstream  third_party/riscv-tests/env/p (default), the real riscv-tests
#             "p" env. Needs Zicsr, trap delivery and mret; the result is
#             written to tohost, which its link.ld places at 0x80001000.
#   raven     test/riscv-tests/env/riscv_test.h. Ad-hoc scaffolding: no CSRs, no traps,
#             PASS/FAIL are an ecall that raven handles itself.
#
# Override the toolchain prefix with CROSS, e.g. CROSS=riscv32-unknown-elf-

set -eu

usage() {
    echo "usage: $0 [--env upstream|raven] --flat|--elf source.S [output]" >&2
    exit 1
}

env=upstream
if [ $# -ge 2 ] && [ "$1" = --env ]; then
    env=$2
    shift 2
fi

[ $# -eq 2 ] || [ $# -eq 3 ] || usage

case "$1" in
    --flat) ext=bin ;;
    --elf)  ext=elf ;;
    *)      usage ;;
esac

src=$2
[ -f "$src" ] || { echo "$0: no such file: $src" >&2; exit 1; }

root=$(cd "$(dirname "$0")/../.." && pwd)
cross=${CROSS:-riscv64-unknown-elf-}

case "$env" in
    raven)
        march=rv32i
        env_dir=$root/test/riscv-tests/env
        link="-Ttext=0x80000000"
        ;;
    upstream)
        # the env touches CSRs, which GCC 12+ only accepts with Zicsr
        march=rv32i_zicsr
        env_dir=$root/third_party/riscv-tests/env/p
        link="-T $env_dir/link.ld"
        ;;
    *)
        usage
        ;;
esac

name=$(basename "$src" .S)
out=${3:-$root/build/riscv-tests/$env/$name.$ext}
mkdir -p "$(dirname "$out")"

# always link an ELF first; for --flat it's an intermediate
elf=$out
[ "$ext" = bin ] && elf=${out%.bin}.elf

# -mno-relax: otherwise the linker rewrites `la` into gp-relative addressing,
# but the tests use gp as TESTNUM, so those addresses come out wrong
# shellcheck disable=SC2086 # $link is deliberately split
"${cross}gcc" -march="$march" -mabi=ilp32 -mno-relax \
    -nostdlib -static $link \
    -I "$env_dir" \
    -I "$root/third_party/riscv-tests/isa/macros/scalar" \
    -o "$elf" "$src"

if [ "$ext" = bin ]; then
    "${cross}objcopy" -O binary "$elf" "$out"
fi

echo "$out"
