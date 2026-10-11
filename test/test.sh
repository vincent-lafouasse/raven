as=riscv32-unknown-elf-as; oc=riscv32-unknown-elf-objcopy
$as -march=rv32i -mabi=ilp32 -o test.o test.s
$oc -O binary -j .text test.o test.bin

./decode_dump test.bin > test.out.s
$as -march=rv32i -mabi=ilp32 -o test.out.o test.out.s
$oc -O binary -j .text test.out.o test.out.bin
cmp test.bin test.out.bin
