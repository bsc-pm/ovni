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

	enum { TYPE=1, A=1, B=2 };

	instr_openmp_type_create(TYPE, "task");
	instr_openmp_task_create(A, TYPE);
	instr_openmp_task_create(B, TYPE);

	instr_openmp_task_execute(A);
	sleep_us(100);
	instr_openmp_task_execute(B);
	sleep_us(100);
	instr_openmp_task_end(B);
	sleep_us(100);
	instr_openmp_task_end(A);

	instr_end();

	return 0;
}
