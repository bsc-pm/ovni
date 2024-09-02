/* Copyright (c) 2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#define _DEFAULT_SOURCE

#include <nosv.h>
#include <nosv/affinity.h>
#include <stdatomic.h>
#include <unistd.h>

#include "common.h"
#include "compat.h"

#define NTASKS 200
atomic_int nstarted = 0;
atomic_int ncompleted = 0;
atomic_int ready = 0;

nosv_task_t tasks[NTASKS];
nosv_cond_t cond;
nosv_mutex_t mutex;

static void
task_body(nosv_task_t task)
{
	UNUSED(task);
	atomic_fetch_add(&nstarted, 1);
	if (nosv_mutex_lock(mutex) != 0)
		die("nosv_mutex_lock failed");

	while (!ready) {
		if (nosv_cond_wait(cond, mutex) != 0)
			die("nosv_cond_wait failed");
	}

	if (nosv_mutex_unlock(mutex) != 0)
		die("nosv_mutex_unlock failed");
}

static void
task_done(nosv_task_t task)
{
	UNUSED(task);
	atomic_fetch_add(&ncompleted, 1);
}

static void
task_broadcast_run(nosv_task_t task)
{
	UNUSED(task);
	if (nosv_mutex_lock(mutex) != 0)
		die("nosv_mutex_lock failed");

	atomic_store(&ready, 1);
	if (nosv_cond_broadcast(cond) != 0)
		die("nosv_cond_broadcast failed");

	if (nosv_mutex_unlock(mutex) != 0)
		die("nosv_mutex_unlock failed");

	if (nosv_mutex_lock(mutex) != 0)
		die("nosv_mutex_lock failed");

	if (nosv_cond_signal(cond) != 0)
		die("nosv_cond_signal failed");

	if (nosv_mutex_unlock(mutex) != 0)
		die("nosv_mutex_unlock failed");
}

int
main(void)
{
	nosv_init();

	nosv_task_type_t task_type;
	nosv_type_init(&task_type, task_body, NULL, task_done, "task", NULL, NULL, NOSV_TYPE_INIT_NONE);

	nosv_task_t task_broadcast;
	nosv_task_type_t task_type_broadcast;
	nosv_type_init(&task_type_broadcast, task_broadcast_run, NULL, NULL, "task_broadcast", NULL, NULL, NOSV_TYPE_INIT_NONE);

	if (nosv_cond_init(&cond, NOSV_COND_NONE) != 0)
		die("nosv_cond_init failed");

	if (nosv_mutex_init(&mutex, NOSV_MUTEX_NONE) != 0)
		die("nosv_mutex_init failed");

	for (int i = 0; i < NTASKS; i++)
		nosv_create(&tasks[i], task_type, 0, NOSV_CREATE_NONE);

	nosv_create(&task_broadcast, task_type_broadcast, 0, NOSV_CREATE_NONE);

	for (int i = 0; i < NTASKS; i++)
		nosv_submit(tasks[i], NOSV_SUBMIT_NONE);

	while (atomic_load(&nstarted) != NTASKS)
		nosv_yield(NOSV_YIELD_NONE);

	nosv_submit(task_broadcast, NOSV_SUBMIT_NONE);

	while (atomic_load(&ncompleted) != NTASKS)
		sleep_us(1000);

	for (int i = 0; i < NTASKS; i++)
		nosv_destroy(tasks[i], NOSV_DESTROY_NONE);

	nosv_destroy(task_broadcast, NOSV_DESTROY_NONE);

	if (nosv_cond_destroy(cond) != 0)
		die("nosv_cond_destroy failed");

	if (nosv_mutex_destroy(mutex) != 0)
		die("nosv_mutex_destroy failed");

	nosv_type_destroy(task_type, NOSV_TYPE_DESTROY_NONE);
	nosv_type_destroy(task_type_broadcast, NOSV_TYPE_DESTROY_NONE);

	nosv_shutdown();

	return 0;
}

