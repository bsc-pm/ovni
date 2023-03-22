/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "instr.h"

int
main(void)
{
	instr_start(0, 0);
	instr_end();

	return 0;
}
