/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "instr_nanos6.h"

int
main(void)
{
	/* Test that a thread ending while the subsystem still has a value in
	 * the stack causes the emulator to fail */

	instr_start(0, 1);

	instr_nanos6_worker_loop_enter();
	/* The thread is left in the worker loop state (should fail) */

	instr_end();

	return 0;
}
