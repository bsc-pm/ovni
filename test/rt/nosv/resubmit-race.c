/* Copyright (c) 2025 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <nosv.h>
#include <nosv/affinity.h>
#include <stdatomic.h>
#include <unistd.h>
#include "compat.h"
#include "common.h"

#define NTASKS 10

atomic_int nr_completed_tasks;
atomic_int completed;

nosv_task_t child_task;
nosv_task_t main_task;

static void
task_run(nosv_task_t task)
{
	UNUSED(task);
	sleep_us(100);
}

static void
task_comp(nosv_task_t task)
{
	int total = atomic_fetch_add(&nr_completed_tasks, 1);
	if (total < NTASKS) {
		nosv_submit(task, NOSV_SUBMIT_NONE);
		/* Wait to give the scheduler the chance to win the race */
		sleep_us(100);
	} else {
		nosv_submit(main_task, NOSV_SUBMIT_UNLOCKED);
	}
}

int
main(void)
{
	nosv_init();

	nosv_attach(&main_task, NULL, NULL, NOSV_ATTACH_NONE);

	nosv_task_type_t task_type;
	nosv_type_init(&task_type, task_run, NULL, task_comp, "child_task", NULL, NULL, NOSV_TYPE_INIT_NONE);
	nosv_create(&child_task, task_type, 0, 0);
	nosv_submit(child_task, NOSV_SUBMIT_NONE);

	nosv_pause(NOSV_PAUSE_NONE);

	nosv_destroy(child_task, NOSV_DESTROY_NONE);

	nosv_detach(NOSV_DETACH_NONE);

	nosv_type_destroy(task_type, NOSV_TYPE_DESTROY_NONE);

	nosv_shutdown();

	return 0;
}
