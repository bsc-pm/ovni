/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdint.h>
#include <stdio.h>
#include "common.h"
#include "emu_prv.h"
#include "instr.h"
#include "instr_nosv.h"

int
main(void)
{
	/* Tests that switching to a nested nOS-V task (inline if0) of the same
	 * type produces a duplicated event in the Paraver trace with the task
	 * type, appid and rank. */

	instr_start(0, 1);
	instr_nosv_init();

	uint32_t typeid = 100;
	uint32_t gid = instr_nosv_type_create((int32_t) typeid);

	/* Create two tasks of the same type */
	instr_nosv_task_create(1, typeid);
	instr_nosv_task_create(2, typeid);

	instr_nosv_task_execute(1, 0);
	/* Change subsystem to prevent duplicates */
	instr_nosv_submit_enter();
	/* Run another nested task with same type id */
	instr_nosv_task_execute(2, 0);

	/* Match the PRV line in the trace */
	FILE *f = fopen("match.sh", "w");
	if (f == NULL)
		die("fopen failed:");

	/* Check the task type */
	int prvtype = PRV_NOSV_TYPE;
	int64_t t = get_delta();
	fprintf(f, "grep ':%" PRIi64 ":%d:%d$' ovni/thread.prv\n", t, prvtype, gid);
	fprintf(f, "grep ':%" PRIi64 ":%d:%d$' ovni/cpu.prv\n", t, prvtype, gid);

	/* Check the task appid */
	prvtype = PRV_NOSV_APPID;
	int appid = 1; /* Starts at one */
	fprintf(f, "grep ':%" PRIi64 ":%d:%d$' ovni/thread.prv\n", t, prvtype, appid);
	fprintf(f, "grep ':%" PRIi64 ":%d:%d$' ovni/cpu.prv\n", t, prvtype, appid);

	/* Check the rank */
	prvtype = PRV_NOSV_RANK;
	int rank = 0 + 1; /* Starts at one */
	fprintf(f, "grep ':%" PRIi64 ":%d:%d$' ovni/thread.prv\n", t, prvtype, rank);
	fprintf(f, "grep ':%" PRIi64 ":%d:%d$' ovni/cpu.prv\n", t, prvtype, rank);
	fclose(f);

	/* Exit from tasks and subsystem */
	instr_nosv_task_end(2, 0);
	instr_nosv_submit_exit();
	instr_nosv_task_end(1, 0);

	instr_end();

	return 0;
}
