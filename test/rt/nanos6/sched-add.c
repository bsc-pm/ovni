/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <nanos6.h>
#include <nanos6/debug.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int ncpus = -1;
static long nruns = 3L;
static long ntasks = 200L;

static atomic_int nhandles = 0;
static void **handle;

#pragma oss task
static void
do_task(int t)
{
	handle[t] = nanos6_get_current_blocking_context();
	if (handle[t] == NULL)
		abort();

	atomic_fetch_add(&nhandles, 1);
	nanos6_block_current_task(handle[t]);
}

static void
do_run(void)
{
	memset(handle, 0, (size_t) ntasks * sizeof(void *));
	atomic_store(&nhandles, 0);

	for (int t = 0; t < ntasks; t++)
		do_task(t);

	/* Wait for all tasks to fill the handle */
	while (atomic_load(&nhandles) < ntasks)
		;

	/* Is ok if we call unblock before the block happens */
	for (int t = 0; t < ntasks; t++) {
		if (handle[t] == NULL)
			abort();

		nanos6_unblock_task(handle[t]);
	}

	#pragma oss taskwait
}

static int
get_ncpus(void)
{
	return (int) nanos6_get_num_cpus();
}

int
main(void)
{
	ncpus = get_ncpus();

	handle = calloc((size_t) ntasks, sizeof(void *));

	if (handle == NULL) {
		perror("calloc failed");
		return -1;
	}

	for (int run = 0; run < nruns; run++)
		do_run();

	return 0;
}
