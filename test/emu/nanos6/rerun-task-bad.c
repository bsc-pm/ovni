/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdint.h>
#include "instr.h"
#include "instr_nanos6.h"

/* Ensure a Nanos6 task cannot be executed again after ending. */

int
main(void)
{
	instr_start(0, 1);

	uint32_t typeid = 666;
	instr_nanos6_type_create(typeid);

	uint32_t taskid = 1;
	instr_nanos6_task_create_and_execute(taskid, typeid);
	instr_nanos6_task_end(taskid);

	/* Run again the same task (should fail) */
	instr_nanos6_task_execute(taskid);
	instr_nanos6_task_end(taskid);

	instr_end();

	return 0;
}
