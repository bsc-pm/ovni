/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#define _DEFAULT_SOURCE

#include <nosv.h>
#include <nosv/affinity.h>
#include <stdatomic.h>
#include <unistd.h>

#include "common.h"

#define NTASKS 200
atomic_int ncompleted = 0;

nosv_task_t tasks[NTASKS];

static void
task_body(nosv_task_t task)
{
	UNUSED(task);
	const uint64_t time_ns = 5000ULL * 1000ULL;
	uint64_t actual_ns = 0;
	nosv_waitfor(time_ns, &actual_ns);
}

static void
task_done(nosv_task_t task)
{
	UNUSED(task);
	atomic_fetch_add(&ncompleted, 1);
}

int
main(void)
{
	nosv_init();

	nosv_task_type_t task_type;
	nosv_type_init(&task_type, task_body, NULL, task_done, "task", NULL, NULL, 0);

	for (int i = 0; i < NTASKS; i++)
		nosv_create(&tasks[i], task_type, 0, 0);

	for (int i = 0; i < NTASKS; i++)
		nosv_submit(tasks[i], 0);

	while (atomic_load(&ncompleted) != NTASKS)
		usleep(1000);

	for (int i = 0; i < NTASKS; i++)
		nosv_destroy(tasks[i], 0);

	nosv_type_destroy(task_type, 0);

	nosv_shutdown();

	return 0;
}
