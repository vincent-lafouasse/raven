#pragma once

#include "types.h"

#define REG_ZERO 0u
#define REG_RA 1u
#define REG_SP 2u
#define REG_GP 3u
#define REG_TP 4u
#define REG_T0 5u
#define REG_T1 6u
#define REG_T2 7u
#define REG_S0 8u
#define REG_S1 9u
#define REG_A0 10u
#define REG_A1 11u
#define REG_A2 12u
#define REG_A3 13u
#define REG_A4 14u
#define REG_A5 15u
#define REG_A6 16u
#define REG_A7 17u
#define REG_S2 18u
#define REG_S3 19u
#define REG_S4 20u
#define REG_S5 21u
#define REG_S6 22u
#define REG_S7 23u
#define REG_S8 24u
#define REG_S9 25u
#define REG_S10 26u
#define REG_S11 27u
#define REG_T3 28u
#define REG_T4 29u
#define REG_T5 30u
#define REG_T6 31u

struct Cpu {
    u32 x[32];
    u32 pc;
};
