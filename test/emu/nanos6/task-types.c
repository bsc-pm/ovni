/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdlib.h>
#include "compat.h"
#include "instr.h"
#include "instr_nanos6.h"

int
main(void)
{
	int rank = atoi(getenv("OVNI_RANK"));
	int nranks = atoi(getenv("OVNI_NRANKS"));

	instr_start(rank, nranks);
	instr_nanos6_init();

	int ntasks = 100;
	int ntypes = 10;

	for (int i = 0; i < ntypes; i++)
		instr_nanos6_type_create(i + 1);

	for (int i = 0; i < ntasks; i++) {
		instr_nanos6_task_create_and_execute(i + 1, (uint32_t) ((i % ntypes) + 1));
		sleep_us(500);
		instr_nanos6_task_end(i + 1);
		instr_nanos6_task_body_exit();
	}

	instr_end();

	return 0;
}
