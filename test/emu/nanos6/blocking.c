/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdint.h>
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

	int us = 500;
	uint32_t typeid = 1;
	uint32_t taskid = 1;

	instr_nanos6_type_create(typeid);

	instr_nanos6_task_create_and_execute(taskid, typeid);
	sleep_us(us);
	instr_nanos6_block_enter();
	instr_nanos6_task_pause(taskid);
	sleep_us(us);
	instr_nanos6_task_resume(taskid);
	instr_nanos6_block_exit();
	sleep_us(us);
	instr_nanos6_task_end(taskid);
	instr_nanos6_task_body_exit();

	instr_end();

	return 0;
}
