/* Copyright (c) 2024-2025 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#define _DEFAULT_SOURCE

#include <nosv.h>
#include <nosv/affinity.h>
#include <nosv/compat.h>
#include <stdatomic.h>
#include <unistd.h>

#include "common.h"
#include "compat.h"

#define NTASKS 200
atomic_int ncompleted = 0;

nosv_task_t tasks[NTASKS];
nosv_barrier_t barrier;

static void
task_body(nosv_task_t task)
{
	UNUSED(task);
	if (nosv_barrier_wait(&barrier) != 0)
		die("nosv_barrier_wait failed");
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

	if (nosv_barrier_init(&barrier, NULL, NTASKS) != 0)
		die("nosv_barrier_init failed");

	for (int i = 0; i < NTASKS; i++)
		nosv_create(&tasks[i], task_type, 0, 0);

	for (int i = 0; i < NTASKS; i++)
		nosv_submit(tasks[i], 0);

	while (atomic_load(&ncompleted) != NTASKS)
		sleep_us(1000);

	for (int i = 0; i < NTASKS; i++)
		nosv_destroy(tasks[i], 0);

	if (nosv_barrier_destroy(&barrier) != 0)
		die("nosv_barrier_destroy failed");

	nosv_type_destroy(task_type, 0);

	nosv_shutdown();

	return 0;
}
