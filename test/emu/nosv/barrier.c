/* Copyright (c) 2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "compat.h"
#include "instr.h"
#include "instr_nosv.h"

/* Test the nosv_barrier_wait() API events, introduced in the nOS-V model 2.1.0 */

int
main(void)
{
	instr_start(0, 1);
	instr_nosv_init();

	instr_nosv_barrier_wait_enter();
	sleep_us(100);
	instr_nosv_barrier_wait_exit();

	instr_end();

	return 0;
}

