/* Copyright (c) 2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "compat.h"
#include "instr.h"
#include "instr_nosv.h"

/* Test the nosv_attach() and nosv_detach() API events, introduced in the nOS-V
 * model 1.1.0 */

int
main(void)
{
	instr_start(0, 1);
	instr_nosv_init();

	instr_nosv_attach_enter();
	sleep_us(100);
	instr_nosv_attach_exit();

	instr_nosv_detach_enter();
	sleep_us(100);
	instr_nosv_detach_exit();

	instr_end();

	return 0;
}
