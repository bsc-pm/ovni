/* Copyright (c) 2022 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "instr_nanos6.h"

int
main(void)
{
	instr_start(0, 1);

	instr_nanos6_worker_loop_enter();
	/* The thread is left in the worker loop state (should fail) */

	instr_end();

	return 0;
}
