/* Copyright (c) 2023-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdint.h>
#include "compat.h"
#include "instr.h"
#include "instr_nosv.h"
#include "emu/task.h"

/* Run two task bodies without pausing the previous one. */

int
main(void)
{
	instr_start(0, 1);
	instr_nosv_init();

	uint32_t typeid = 100;

	instr_nosv_type_create((int32_t) typeid);
	instr_nosv_task_create(1, typeid);
	instr_nosv_task_create(2, typeid);

	instr_nosv_submit_enter();
	instr_nosv_task_execute(1, 0);
	{
		instr_nosv_submit_enter();
		instr_nosv_task_execute(2, 0); /* Must panic */
		instr_nosv_task_end(2, 0);
		instr_nosv_submit_exit();
	}
	instr_nosv_task_end(1, 0);
	instr_nosv_submit_exit();

	instr_end();

	return 0;
}
