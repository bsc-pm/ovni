/* Copyright (c) 2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdint.h>
#include <stdlib.h>
#include "compat.h"
#include "instr.h"
#include "instr_nanos6.h"

/* Cause the sort module to write zeros in some rows when the breakdown model is
 * enabled, which shouldn't cause a panic. */
int
main(void)
{
	int rank = atoi(getenv("OVNI_RANK"));
	int nranks = atoi(getenv("OVNI_NRANKS"));

	if (nranks < 2)
		die("need at least two ranks");

	instr_start(rank, nranks);
	instr_nanos6_init();

	/* Change the subsystem in a single thread, while the rest remain NULL.
	 * This shouldn't cause a panic, as writting zero is ok for the other
	 * sort outputs. */
	if (rank == 0) {
		instr_nanos6_worker_loop_enter();
		instr_nanos6_worker_loop_exit();
	}

	instr_end();

	return 0;
}
