/* Copyright (c) 2025 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdlib.h>
#include "compat.h"
#include "instr.h"

int
main(void)
{
	int rank = atoi(getenv("OVNI_RANK"));
	int nranks = atoi(getenv("OVNI_NRANKS"));

	if (nranks < 2)
		die("need at least 2 ranks");

	char hostname[OVNI_MAX_HOSTNAME];

	if (gethostname(hostname, OVNI_MAX_HOSTNAME) != 0)
		die("gethostname failed");

	ovni_version_check();
	ovni_proc_init(1, hostname, getpid());
	ovni_thread_init(get_tid());

	/* Wrongly set the logical index to 0 always */
	ovni_add_cpu(0, rank);

	instr_thread_execute(rank, -1, 0);

	sleep_us(50);

	instr_end();

	return 0;
}
