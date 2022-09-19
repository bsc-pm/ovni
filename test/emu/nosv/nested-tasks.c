/* Copyright (c) 2021-2022 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "instr_nosv.h"

static void
create_and_run(int32_t id, uint32_t typeid, int us)
{
	instr_nosv_task_create(id, typeid);
	instr_nosv_task_execute(id);
	usleep(us);
}

int
main(void)
{
	instr_start(0, 1);

	int ntasks = 100;
	uint32_t typeid = 1;

	instr_nosv_type_create(typeid);

	/* Create and run the tasks, one nested into another */
	for(int i = 0; i < ntasks; i++)
		create_and_run(i + 1, typeid, 500);

	/* End the tasks in the opposite order */
	for(int i = ntasks - 1; i >= 0; i--)
		instr_nosv_task_end(i + 1);

	instr_end();

	return 0;
}

