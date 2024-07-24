/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdint.h>
#include "compat.h"
#include "instr.h"
#include "instr_nanos6.h"

int
main(void)
{
	instr_start(0, 1);
	instr_nanos6_init();

	int ntasks = 100;
	uint32_t typeid = 1;

	instr_nanos6_type_create((int32_t) typeid);

	/* Create and run the tasks, one nested into another */
	for (int i = 0; i < ntasks; i++) {
		instr_nanos6_handle_task_enter();
		instr_nanos6_task_create_and_execute(i + 1, typeid);
		sleep_us(500);
		instr_nanos6_task_pause(i + 1);
	}

	/* End the tasks in the opposite order */
	for (int i = ntasks - 1; i >= 0; i--) {
		instr_nanos6_task_resume(i + 1);
		instr_nanos6_task_end(i + 1);
		instr_nanos6_task_body_exit();
		instr_nanos6_handle_task_exit();
	}

	instr_end();

	return 0;
}
