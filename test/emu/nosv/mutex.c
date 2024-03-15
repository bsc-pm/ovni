/* Copyright (c) 2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "compat.h"
#include "instr.h"
#include "instr_nosv.h"

/* Test the nosv_mutex_lock(), nosv_mutex_trylock() and nosv_mutex_unlock() API
 * events, introduced in the nOS-V model 2.1.0 */

int
main(void)
{
	instr_start(0, 1);
	instr_nosv_init();

	instr_nosv_mutex_lock_enter();
	sleep_us(100);
	instr_nosv_mutex_lock_exit();

	instr_nosv_mutex_trylock_enter();
	sleep_us(100);
	instr_nosv_mutex_trylock_exit();

	instr_nosv_mutex_unlock_enter();
	sleep_us(100);
	instr_nosv_mutex_unlock_exit();

	instr_end();

	return 0;
}
