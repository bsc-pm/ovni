/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdio.h>
#include "common.h"
#include "emu/emu_prv.h"
#include "instr.h"
#include "instr_nanos6.h"

/* This test attempts to create holes in the breakdown trace by creating a
 * thread that emits the first Nanos6 event later than the ovni thread execute
 * event OHx. When the breakdown model is enabled, the output breakdown PRV
 * trace should contain already a value for time=0, which is when OHx is
 * emitted. We also test that on 6W[ we emit another non-zero state.
 *
 *               .              .
 *               OHx            6W[
 * --------------x--------------x--------------> time
 *               .              .
 *               .              .
 * breakdown ----A--------------B--------------
 *               .              .
 *
 * We check that A and B are not emitting 0. */

int
main(void)
{
	instr_start(0, 1);
	instr_nanos6_init();

	/* Match the PRV line in the trace */
	FILE *f = fopen("match.sh", "w");
	if (f == NULL)
		die("fopen failed:");

	/* Ensure non-zero for A (first event) */
	fprintf(f, "grep ':%" PRIi64 ":%d:[^0][0-9]*$' ovni/nanos6-breakdown.prv\n",
			get_delta(), PRV_NANOS6_BREAKDOWN);

	instr_nanos6_worker_loop_enter();

	/* And for B */
	fprintf(f, "grep ':%" PRIi64 ":%d:[^0][0-9]*$' ovni/nanos6-breakdown.prv\n",
			get_delta(), PRV_NANOS6_BREAKDOWN);

	instr_nanos6_worker_loop_exit();

	instr_end();

	return 0;
}
