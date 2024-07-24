/* Copyright (c) 2023-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdint.h>
#include "compat.h"
#include "instr.h"
#include "instr_nanos6.h"

/* Ensure that we can accept a Nanos6 task model in which the parent gets paused
 * before the if0 task begins. */

int
main(void)
{
	instr_start(0, 1);
	instr_nanos6_init();

	int ntasks = 100;
	uint32_t typeid = 1;

	instr_nanos6_type_create((int32_t) typeid);

	/* Create and run the tasks, one nested into another */
	for (int32_t id = 1; id <= ntasks; id++) {
		instr_nanos6_handle_task_enter();
		instr_nanos6_task_create_begin();
		instr_nanos6_task_create(id, typeid);
		instr_nanos6_task_create_end();
		instr_nanos6_task_body_enter();
		instr_nanos6_task_execute(id);
		sleep_us(500);
		instr_nanos6_task_pause(id);
	}

	/* End the tasks in the opposite order */
	for (int32_t id = ntasks; id >= 1; id--) {
		instr_nanos6_task_resume(id);
		instr_nanos6_task_end(id);
		instr_nanos6_task_body_exit();
		instr_nanos6_handle_task_exit();
	}

	instr_end();

	return 0;
}
