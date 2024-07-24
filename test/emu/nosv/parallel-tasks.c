/* Copyright (c) 2023-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdint.h>
#include "compat.h"
#include "instr.h"
#include "instr_nosv.h"
#include "emu/task.h"

int
main(void)
{
	instr_start(0, 1);
	instr_nosv_init();

	uint32_t nbodies = 100;
	uint32_t typeid = 100;
	uint32_t taskid = 200;

	instr_nosv_type_create((int32_t) typeid);
	instr_nosv_task_create_par(taskid, typeid);

	/* Create and run the tasks, one nested into another */
	for (uint32_t bodyid = 1; bodyid <= nbodies; bodyid++) {
		/* Change subsystem to prevent duplicates */
		instr_nosv_submit_enter();
		instr_nosv_task_execute(taskid, bodyid);
		sleep_us(100);
		instr_nosv_task_end(taskid, bodyid);
		instr_nosv_submit_exit();
	}

	instr_end();

	return 0;
}
