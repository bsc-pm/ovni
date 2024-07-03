/* Copyright (c) 2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdlib.h>
#include "compat.h"
#include "instr.h"
#include "instr_openmp.h"

int
main(void)
{
	instr_start(0, 1);
	instr_openmp_init();

	enum { TASK=1, WS1=2, WS2=3 };
	instr_openmp_type_create(TASK, "main task");
	instr_openmp_type_create(WS1, "outer for");
	instr_openmp_type_create(WS2, "inner for");

	instr_openmp_task_create(1, TASK);
	instr_openmp_task_execute(1);
	sleep_us(100);
	for (int i = 0; i < 3; i++) {
		instr_openmp_ws_enter(WS1);
		sleep_us(10);
		{
			instr_openmp_ws_enter(WS2);
			sleep_us(10);
			instr_openmp_ws_exit(WS2);
		}
		sleep_us(10);
		instr_openmp_ws_exit(WS1);
		sleep_us(10);
	}
	sleep_us(10);
	instr_openmp_task_end(1);
	sleep_us(10);
	/* Another task from the same type */
	instr_openmp_task_create(2, TASK);
	instr_openmp_task_execute(2);
	sleep_us(100);
	instr_openmp_task_end(2);

	instr_end();

	return 0;
}
