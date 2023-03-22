/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdint.h>
#include <stdlib.h>
#include "compat.h"
#include "instr.h"
#include "instr_nosv.h"

int
main(void)
{
	int rank = atoi(getenv("OVNI_RANK"));
	int nranks = atoi(getenv("OVNI_NRANKS"));
	instr_start(rank, nranks);

	int us = 500;
	uint32_t typeid = 1;

	instr_nosv_type_create(typeid);
	instr_nosv_task_create(1, typeid);
	instr_nosv_task_execute(1);
	sleep_us(us);
	instr_nosv_task_pause(1);
	sleep_us(us);
	instr_nosv_task_resume(1);
	sleep_us(us);
	instr_nosv_task_end(1);

	instr_end();

	return 0;
}
