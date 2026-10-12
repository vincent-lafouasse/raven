prefix=riscv64-unknown-elf
as=${prefix}-as
oc=${prefix}-objcopy

build=build

mkdir -p ${build}

$as -march=rv32i -mabi=ilp32 -o ${build}/test.o test.s
$oc -O binary -j .text ${build}/test.o ${build}/test.bin

decoder="../${build}/debug/bin/raven-dis"

$decoder ${build}/test.bin > ${build}/test.out.s
$as -march=rv32i -mabi=ilp32 -o ${build}/test.out.o ${build}/test.out.s
$oc -O binary -j .text ${build}/test.out.o ${build}/test.out.bin
cmp ${build}/test.bin ${build}/test.out.bin
