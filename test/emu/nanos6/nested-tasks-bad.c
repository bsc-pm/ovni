/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "instr_nanos6.h"

int
main(void)
{
	instr_start(0, 1);

	uint32_t typeid = 666;
	instr_nanos6_type_create(typeid);

	uint32_t taskid = 1;
	instr_nanos6_task_create_and_execute(taskid, typeid);

	/* Run another nested task with same id (should fail) */
	instr_nanos6_task_execute(taskid);

	instr_end();

	return 0;
}
