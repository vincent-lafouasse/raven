#pragma once

#include "trap.h"

enum TrapStatus csr_read();
enum TrapStatus csr_write();

// non-architectural, diagnostics only
const char* csr_name(u32 csr);   // NULL if not a standard CSR
