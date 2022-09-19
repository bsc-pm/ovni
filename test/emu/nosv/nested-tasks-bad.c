/* Copyright (c) 2021-2022 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "instr_nosv.h"

int
main(void)
{
	instr_start(0, 1);

	uint32_t typeid = 666;
	instr_nosv_type_create(typeid);

	uint32_t taskid = 1;
	instr_nosv_task_create(taskid, typeid);
	instr_nosv_task_execute(taskid);
	/* Run another nested task with same id (should fail) */
	instr_nosv_task_execute(taskid);

	instr_end();

	return 0;
}
