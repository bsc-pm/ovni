/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <nosv.h>
#include "compat.h"
#include "common.h"

static void
worker_run(nosv_task_t task)
{
	UNUSED(task);
	sleep_us(50);
}

static void
worker_comp(nosv_task_t task)
{
	UNUSED(task);
	nosv_destroy(task, NOSV_DESTROY_NONE);
}

/* Runs an inline task inside another one */

int
main(void)
{
	nosv_init();

	nosv_task_t ext_task;
	if (nosv_attach(&ext_task, NULL, NULL, NOSV_ATTACH_NONE) != 0)
		die("nosv_attach failed");

	nosv_task_type_t worker;
	if (nosv_type_init(&worker, worker_run, NULL, worker_comp, "worker", NULL, NULL, NOSV_TYPE_INIT_NONE) != 0)
		die("nosv_type_init failed");

	nosv_task_t worker_task;
	nosv_create(&worker_task, worker, 0, NOSV_CREATE_NONE);

	nosv_submit(worker_task, NOSV_SUBMIT_INLINE);
	nosv_type_destroy(worker, NOSV_TYPE_DESTROY_NONE);

	nosv_detach(NOSV_DETACH_NONE);

	nosv_shutdown();
}
