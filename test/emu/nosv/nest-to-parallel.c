/* Copyright (c) 2023-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdint.h>
#include "compat.h"
#include "instr.h"
#include "instr_nosv.h"
#include "emu/task.h"

/* Run a normal task body (taskid=1,bodyid=0) and before it finishes, switch to
 * another body from a parallel task (taskid=2,bodyid=2). This is valid. */

int
main(void)
{
	instr_start(0, 1);
	instr_nosv_init();

	uint32_t typeid = 100;

	instr_nosv_type_create((int32_t) typeid);
	instr_nosv_task_create(1, typeid);
	instr_nosv_task_create_par(2, typeid);

	instr_nosv_task_execute(1, 0);
	{
		instr_nosv_task_pause(1, 0);
		instr_nosv_submit_enter();
		{
			instr_nosv_task_execute(2, 2);
			sleep_us(10);
			instr_nosv_task_end(2, 2);
		}
		instr_nosv_submit_exit();
		instr_nosv_task_resume(1, 0);
	}
	instr_nosv_task_end(1, 0);

	instr_end();

	return 0;
}
