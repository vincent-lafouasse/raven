base=riscv64-unknown-elf
as=${base}-as
oc=${base}-objcopy
$as -march=rv32i -mabi=ilp32 -o test.o test.s
$oc -O binary -j .text test.o test.bin

decoder="../build/debug/bin/raven-dis"

$decoder test.bin > test.out.s
$as -march=rv32i -mabi=ilp32 -o test.out.o test.out.s
$oc -O binary -j .text test.out.o test.out.bin
cmp test.bin test.out.bin
