/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "instr_nosv.h"

int
main(void)
{
	int rank = atoi(getenv("OVNI_RANK"));
	int nranks = atoi(getenv("OVNI_NRANKS"));

	instr_start(rank, nranks);

	int ntasks = 100;
	int ntypes = 10;

	for (int i = 0; i < ntypes; i++)
		instr_nosv_type_create(i + 1);

	for (int i = 0; i < ntasks; i++) {
		instr_nosv_task_create(i + 1, (i % ntypes) + 1);
		instr_nosv_task_execute(i + 1);
		usleep(500);
		instr_nosv_task_end(i + 1);
	}

	instr_end();

	return 0;
}
