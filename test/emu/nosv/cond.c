/* Copyright (c) 2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "compat.h"
#include "instr.h"
#include "instr_nosv.h"

/* Test the nosv_cond_wait(), nosv_cond_broadcast() and nosv_cond_signal() API
 * events, introduced in the nOS-V model 2.4.0 */

int
main(void)
{
	instr_start(0, 1);
	instr_nosv_init();

	instr_nosv_cond_wait_enter();
	sleep_us(100);
	instr_nosv_cond_wait_exit();

	instr_nosv_cond_broadcast_enter();
	sleep_us(100);
	instr_nosv_cond_broadcast_exit();

	instr_nosv_cond_signal_enter();
	sleep_us(100);
	instr_nosv_cond_signal_exit();

	instr_end();

	return 0;
}
