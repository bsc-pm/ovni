/* Copyright (c) 2022 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#define _POSIX_C_SOURCE 2

#include <nanos6.h>
#include <nanos6/debug.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static int ncpus = -1;
static long nruns = 3L;
static long ntasks = 10000L;

static atomic_int wait = 0;
static void **handle;

#pragma oss task
static void
do_task(int t)
{
	if(atomic_load(&wait))
	{
		handle[t] = nanos6_get_current_blocking_context();
		nanos6_block_current_task(handle[t]);
	}
}

static void
do_run(void)
{
	memset(handle, 0, ntasks * sizeof(void *));
	atomic_fetch_add(&wait, 1);

	for(int t = 0; t < ntasks; t++)
		do_task(t);

	atomic_fetch_sub(&wait, 1);

	for(int t = 0; t < ntasks; t++)
	{
		if (handle[t]) {
			nanos6_unblock_task(handle[t]);
		}
	}

	#pragma oss taskwait
}

static int get_ncpus(void)
{
	return (int) nanos6_get_num_cpus();
}

int
main(void)
{
	ncpus = get_ncpus();

	handle = calloc(ntasks, sizeof(void *));

	if(handle == NULL)
	{
		perror("calloc failed");
		return -1;
	}

	printf("%s,%s,%s,%s\n", "run", "ntasks", "time", "time_per_task_per_cpu");
	for(int run = 0; run < nruns; run++)
		do_run();

	return 0;
}
