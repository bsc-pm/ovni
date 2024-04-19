/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdint.h>
#include <stdio.h>
#include "common.h"
#include "emu_prv.h"
#include "instr.h"
#include "instr_nanos6.h"
#include "nanos6/nanos6_priv.h"

int
main(void)
{
	instr_start(0, 1);
	instr_nanos6_init();

	/* Until here, the nanos6 model has not been connected to the
	 * patchbay yet. The next event will cause the subsystem mux to
	 * be connected to the patchbay. We need to ensure that at that
	 * moment the select channel is evaluated, otherwise the mux
	 * will remain selecting a null input until the thread state
	 * changes. */

	instr_nanos6_worker_loop_enter();

	/* Match the PRV line in the trace */
	FILE *f = fopen("match.sh", "w");
	if (f == NULL)
		die("fopen failed:");

	/* Ensure that after the Nanos6 connect phase, the CPU subsystem mux has
	 * selected the correct input, based on the running thread */
	int type = PRV_NANOS6_SUBSYSTEM;
	int64_t t = get_delta();
	int value = ST_WORKER_LOOP;
	fprintf(f, "grep ':%" PRIi64 ":%d:%d$' ovni/thread.prv\n", t, type, value);
	fprintf(f, "grep ':%" PRIi64 ":%d:%d$' ovni/cpu.prv\n", t, type, value);

	instr_nanos6_worker_loop_exit();

	/* Also test when exitting the stacked subsystem */
	t = get_delta();
	value = 0;
	fprintf(f, "grep ':%" PRIi64 ":%d:%d$' ovni/thread.prv\n", t, type, value);
	fprintf(f, "grep ':%" PRIi64 ":%d:%d$' ovni/cpu.prv\n", t, type, value);

	fclose(f);

	instr_end();

	return 0;
}
