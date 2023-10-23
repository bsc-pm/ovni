/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <nosv.h>
#include <nosv/affinity.h>
#include <stdatomic.h>
#include <unistd.h>
#include "compat.h"
#include "common.h"

#define NTASKS 200
#define DEGREE 100

atomic_int nr_completed_tasks;

nosv_task_t tasks[NTASKS];
nosv_task_t main_task;

static void
task_run(nosv_task_t task)
{
	UNUSED(task);
	sleep_us(5);
}

static void
task_comp(nosv_task_t task)
{
	UNUSED(task);

	int total = atomic_fetch_add(&nr_completed_tasks, 1);
	if (total == NTASKS - 1) {
		nosv_submit(main_task, NOSV_SUBMIT_UNLOCKED);
	}
}

int
main(void)
{
	nosv_init();

	nosv_attach(&main_task, NULL, NULL, NOSV_ATTACH_NONE);

	nosv_task_type_t task_type;
	nosv_type_init(&task_type, task_run, NULL, task_comp, "task", NULL, NULL, NOSV_TYPE_INIT_NONE);

	for (int t = 0; t < NTASKS; t++) {
		nosv_create(&tasks[t], task_type, 0, NOSV_CREATE_PARALLEL);
		nosv_set_task_degree(tasks[t], DEGREE);
		nosv_submit(tasks[t], NOSV_SUBMIT_NONE);
	}

	nosv_pause(NOSV_PAUSE_NONE);

	for (int t = 0; t < NTASKS; t++)
		nosv_destroy(tasks[t], NOSV_DESTROY_NONE);

	nosv_detach(NOSV_DETACH_NONE);

	nosv_type_destroy(task_type, NOSV_TYPE_DESTROY_NONE);

	nosv_shutdown();

	return 0;
}
