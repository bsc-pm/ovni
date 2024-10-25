/* Copyright (c) 2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdlib.h>
#include "compat.h"
#include "instr.h"

/* Ensure we can emit CPUs from multiple threads of the same loom */

static inline void
start(int rank, int nranks)
{
	char hostname[OVNI_MAX_HOSTNAME];

	if (gethostname(hostname, OVNI_MAX_HOSTNAME) != 0)
		die("gethostname failed");

	ovni_version_check();

	/* Only one loom */
	ovni_proc_init(1, hostname, getpid());
	ovni_thread_init(get_tid());
	ovni_proc_set_rank(rank, nranks);

	/* Only emit a subset of CPUs up to the rank number */
	for (int i = 0; i <= rank; i++)
		ovni_add_cpu(i, i);

	int curcpu = rank;

	dbg("thread %d has cpu %d (ncpus=%d)",
			get_tid(), curcpu, nranks);

	instr_thread_execute(curcpu, -1, 0);
}


int
main(void)
{
	int rank = atoi(getenv("OVNI_RANK"));
	int nranks = atoi(getenv("OVNI_NRANKS"));

	start(rank, nranks);

	sleep_us(50 * 1000);

	instr_end();

	return 0;
}
