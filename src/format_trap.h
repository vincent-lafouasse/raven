#pragma once

#include "trap.h"

#define TRAP_STR_LEN 256

void format_trap(struct Trap trap, char out[static TRAP_STR_LEN]);
