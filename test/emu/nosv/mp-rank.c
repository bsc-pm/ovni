/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "compat.h"
#include "instr_nosv.h"

static void
task(uint32_t id, uint32_t typeid, int us)
{
	instr_nosv_task_create(id, typeid);
	instr_nosv_task_execute(id, 0);
	sleep_us(us);
	instr_nosv_task_end(id, 0);
}

int
main(void)
{
	int rank = atoi(getenv("OVNI_RANK"));
	int nranks = atoi(getenv("OVNI_NRANKS"));
	uint32_t typeid = 1;

	instr_start(rank, nranks);
	instr_nosv_init();

	instr_nosv_type_create((int32_t) typeid);

	/* Create some fake nosv tasks */
	for (int i = 0; i < 10; i++)
		task((uint32_t) i + 1, typeid, 5000);

	instr_end();

	return 0;
}
