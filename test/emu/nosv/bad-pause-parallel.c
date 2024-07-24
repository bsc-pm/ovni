/* Copyright (c) 2023-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdint.h>
#include "compat.h"
#include "instr.h"
#include "instr_nosv.h"
#include "emu/task.h"

/* Run a parallel task body (taskid=1,bodyid=1) and attempt to pause it. This is
 * not valid because nOS-V cannot pause a parallel task or execute an inline
 * task from a parallel task (which requires the previous one to pause). */

int
main(void)
{
	instr_start(0, 1);
	instr_nosv_init();

	uint32_t typeid = 100;

	instr_nosv_type_create((int32_t) typeid);
	instr_nosv_task_create_par(1, typeid);
	instr_nosv_submit_enter();
	instr_nosv_task_execute(1, 1);
	instr_nosv_task_pause(1, 1); /* Should fail */

	instr_nosv_task_resume(1, 1);
	instr_nosv_task_end(1, 1);
	instr_nosv_submit_exit();

	instr_end();

	return 0;
}
