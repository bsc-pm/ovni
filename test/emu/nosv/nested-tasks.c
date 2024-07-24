/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdint.h>
#include "compat.h"
#include "instr.h"
#include "instr_nosv.h"

/* Create and run tasks, one nested into another */

int
main(void)
{
	instr_start(0, 1);
	instr_nosv_init();

	int ntasks = 100;
	uint32_t typeid = 1;

	instr_nosv_type_create((int32_t) typeid);

	for (int id = 1; id <= ntasks; id++)
		instr_nosv_task_create((uint32_t) id, typeid);

	for (int id = 1; id <= ntasks; id++) {
		instr_nosv_task_execute((uint32_t) id, 0);
		instr_nosv_task_pause((uint32_t) id, 0);
		instr_nosv_submit_enter();
	}

	for (int id = ntasks; id >= 1; id--) {
		instr_nosv_submit_exit();
		instr_nosv_task_resume((uint32_t) id, 0);
		instr_nosv_task_end((uint32_t) id, 0);
	}

	instr_end();

	return 0;
}
