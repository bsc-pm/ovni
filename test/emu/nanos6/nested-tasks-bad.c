/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdint.h>
#include "instr.h"
#include "instr_nanos6.h"

int
main(void)
{
	instr_start(0, 1);
	instr_nanos6_init();

	uint32_t typeid = 666;
	instr_nanos6_type_create((int32_t) typeid);

	uint32_t taskid = 1;
	instr_nanos6_task_create_and_execute((int32_t) taskid, typeid);

	/* Run another nested task with same id (should fail) */
	instr_nanos6_task_execute((int32_t) taskid);

	instr_end();

	return 0;
}
