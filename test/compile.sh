#!/bin/sh
# Builds one riscv-tests source (or any RV32I .S) for raven.
#
#   test/compile.sh --elf  path/to/test.S [out.elf]
#   test/compile.sh --flat path/to/test.S [out.bin]
#
# Output defaults to build/riscv-tests/<name>.elf or .bin at the repo root.
# The program is linked at 0x80000000 (RAM_BASE) against raven's own
# environment in test/env/riscv_test.h. A flat binary is the linked ELF
# flattened with objcopy, so _start ends up as its first byte.
#
# Override the toolchain prefix with CROSS, e.g. CROSS=riscv32-unknown-elf-

set -eu

usage() {
    echo "usage: $0 --flat|--elf source.S [output]" >&2
    exit 1
}

[ $# -eq 2 ] || [ $# -eq 3 ] || usage

case "$1" in
    --flat) ext=bin ;;
    --elf)  ext=elf ;;
    *)      usage ;;
esac

src=$2
[ -f "$src" ] || { echo "$0: no such file: $src" >&2; exit 1; }

root=$(cd "$(dirname "$0")/.." && pwd)
cross=${CROSS:-riscv64-unknown-elf-}

name=$(basename "$src" .S)
out=${3:-$root/build/riscv-tests/$name.$ext}
mkdir -p "$(dirname "$out")"

# always link an ELF first; for --flat it's an intermediate
elf=$out
[ "$ext" = bin ] && elf=${out%.bin}.elf

"${cross}gcc" -march=rv32i -mabi=ilp32 \
    -nostdlib -static -Ttext=0x80000000 \
    -I "$root/test/env" \
    -I "$root/third_party/riscv-tests/isa/macros/scalar" \
    -o "$elf" "$src"

if [ "$ext" = bin ]; then
    "${cross}objcopy" -O binary "$elf" "$out"
fi

echo "$out"
