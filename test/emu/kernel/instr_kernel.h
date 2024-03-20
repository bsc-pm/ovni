/* Copyright (c) 2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef INSTR_KERNEL_H
#define INSTR_KERNEL_H

#include "instr.h"

static inline void
instr_kernel_init(void)
{
	instr_require("kernel");
}

INSTR_0ARG(instr_kernel_cs_out, "KCO")
INSTR_0ARG(instr_kernel_cs_in,  "KCI")

#endif /* INSTR_KERNEL_H */
