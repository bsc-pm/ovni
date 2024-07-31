/* Copyright (c) 2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <nosv.h>
#include <nosv/affinity.h>
#include <stdatomic.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "compat.h"

int done = 0;
unsigned long ncs = 0;
unsigned long nflush = 0;

static void
task_run(nosv_task_t task)
{
	UNUSED(task);

	struct timespec one_ns = { .tv_sec = 0, .tv_nsec = 1 };

	for (unsigned long i = 1; i <= ncs; i++) {
		nanosleep(&one_ns, NULL);
		if (i % nflush == 0) {
			info("flushing at %lu", i);
			if (nosv_waitfor(1, NULL) != 0)
				die("nosv_waitfor failed");
		}
	}

	done = 1;
}

int main(int argc, char *argv[])
{
	if (argc != 3)
		die("usage: %s <num-cs> <num-flush>\n", argv[0]);

	ncs = (unsigned long) atol(argv[1]);
	nflush = (unsigned long) atol(argv[2]);

	if (nosv_init())
		die("nosv_init failed");

	nosv_task_type_t task_type;
	if (nosv_type_init(&task_type, task_run, NULL, NULL, "task", NULL, NULL, NOSV_TYPE_INIT_NONE))
		die("nosv_type_init failed");

	nosv_task_t task;
	if (nosv_create(&task, task_type, 0, NOSV_CREATE_NONE))
		die("nosv_create failed");

	if (nosv_submit(task, NOSV_SUBMIT_IMMEDIATE))
		die("nosv_submit failed");

	/* Wait for the task to finish */
	while (!done)
		sleep_us(10000);

	if (nosv_destroy(task, NOSV_DESTROY_NONE))
		die("nosv_destroy failed");

	if (nosv_type_destroy(task_type, NOSV_TYPE_DESTROY_NONE))
		die("nosv_type_destroy failed");

	if (nosv_shutdown())
		die("nosv_shutdown failed");

	return 0;
}
