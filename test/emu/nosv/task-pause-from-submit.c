/* Copyright (c) 2023-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdint.h>
#include <stdio.h>
#include "common.h"
#include "emu_prv.h"
#include "instr.h"
#include "instr_nosv.h"
#include "emu/nosv/nosv_priv.h"

int
main(void)
{
	/* Tests the case where a task is paused from nosv_submit():
	 *
	 * A VTp event causes a pop of the subsystem channel, which is expected
	 * to be ST_TASK_RUNNING. This is not true in the case of a blocking
	 * submit (NOSV_SUBMIT_BLOCKING), which calls nosv_pause from inside
	 * nosv_submit, causing the transition to happen from the unexpected
	 * ST_API_SUBMIT state.
	 */

	instr_start(0, 1);
	instr_nosv_init();

	/* Match the PRV line in the trace */
	FILE *f = fopen("match.sh", "w");
	if (f == NULL)
		die("fopen failed:");

	uint32_t typeid = 100;
	instr_nosv_type_create(typeid);

	instr_nosv_task_create(1, typeid);

	instr_nosv_task_execute(1, 0);

	/* When starting a task, it must cause the subsystems to enter
	 * the "Task: In body" state */
	int prvtype = PRV_NOSV_SUBSYSTEM;
	int st = ST_TASK_BODY;
	fprintf(f, "grep ':%" PRIi64 ":%d:%d$' ovni/thread.prv\n", get_delta(), prvtype, st);
	fprintf(f, "grep ':%" PRIi64 ":%d:%d$' ovni/cpu.prv\n", get_delta(), prvtype, st);

	instr_nosv_submit_enter(); /* Blocking submit */
	instr_nosv_task_pause(1, 0);

	/* Should be left in the submit state, so no state transition in
	 * subsystems view */
	fprintf(f, "! grep ':%" PRIi64 ":%d:[0-9]*$' ovni/thread.prv\n", get_delta(), prvtype);
	fprintf(f, "! grep ':%" PRIi64 ":%d:[0-9]*$' ovni/cpu.prv\n", get_delta(), prvtype);

	/* But the task state must be set to pause, so the task id
	 * must be null */
	prvtype = PRV_NOSV_TASKID;
	fprintf(f, "grep ':%" PRIi64 ":%d:0$' ovni/thread.prv\n", get_delta(), prvtype);
	fprintf(f, "grep ':%" PRIi64 ":%d:0$' ovni/cpu.prv\n", get_delta(), prvtype);

	instr_nosv_submit_exit();
	instr_nosv_task_resume(1, 0);
	instr_nosv_task_end(1, 0);

	fclose(f);

	instr_end();

	return 0;
}
