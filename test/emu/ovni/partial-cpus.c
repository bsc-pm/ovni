/* Copyright (c) 2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "common.h"
#include "compat.h"
#include "instr.h"
#include "ovni.h"

/* Ensures that we can emit a partial list of CPUs. */
int
main(void)
{
	int rank = atoi(getenv("OVNI_RANK"));
	int nranks = atoi(getenv("OVNI_NRANKS"));

	if (nranks < 2)
		die("needs at least 2 ranks");

	ovni_proc_init(1, "loom0", getpid());
	ovni_thread_init(get_tid());

	int i = rank;

	/* Only emit one CPU per thread */
	ovni_add_cpu(i, i);

	instr_thread_execute(i, -1, 0);
	instr_end();

	return 0;
}
