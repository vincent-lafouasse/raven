// raven's test environment for riscv-tests (replaces env/p/riscv_test.h)
//
// Upstream's env boots a real machine: it zeroes registers, sets up traps,
// memory protection and privilege modes with CSR instructions, then reports
// results through a `tohost` mailbox. raven sets up its initial state in C
// and has no CSRs, so all of that goes away. What is left:
//
//   - execution starts at _start, the first byte of .text (0x80000000)
//   - pass/fail exit via `ecall` with a7 = 93 (Linux `exit`), code in a0:
//     0 on pass, (testnum << 1) | 1 on failure
//   - the whole binary is plain RV32I (plus `fence`, a no-op for raven)

#ifndef RAVEN_RISCV_TEST_H
#define RAVEN_RISCV_TEST_H

// "this is a user-mode integer test". Upstream defines an `init` macro here
// that its boot code calls; we have no boot code, so nothing to do.
#define RVTEST_RV32U
#define RVTEST_RV64U RVTEST_RV32U

// register holding the current test case number, set by test_macros.h
#define TESTNUM gp

#define RVTEST_CODE_BEGIN \
    .text;                \
    .globl _start;        \
_start:

// never reached: every test ends in RVTEST_PASS or RVTEST_FAIL
#define RVTEST_CODE_END

#define RVTEST_PASS \
    fence;          \
    li TESTNUM, 1;  \
    li a7, 93;      \
    li a0, 0;       \
    ecall

// TESTNUM is never 0 here, test_macros.h sets it before every case
#define RVTEST_FAIL              \
    fence;                       \
    sll TESTNUM, TESTNUM, 1;     \
    or TESTNUM, TESTNUM, 1;      \
    li a7, 93;                   \
    addi a0, TESTNUM, 0;         \
    ecall

// input data for the load/store tests
#define RVTEST_DATA_BEGIN \
    .data;                \
    .balign 16;
#define RVTEST_DATA_END

#endif
