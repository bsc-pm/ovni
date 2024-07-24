/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdint.h>
#include "instr.h"
#include "instr_nosv.h"

int
main(void)
{
	instr_start(0, 1);
	instr_nosv_init();

	uint32_t typeid = 666;
	instr_nosv_type_create((int32_t) typeid);

	instr_nosv_task_create(1, typeid);
	instr_nosv_task_execute(1, 0);
	instr_nosv_task_pause(1, 0);

	/* Run another nested task with same id (should fail) */
	instr_nosv_task_execute(1, 0);

	instr_end();

	return 0;
}
