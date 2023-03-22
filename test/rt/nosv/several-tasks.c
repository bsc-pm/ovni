/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <nosv.h>
#include <stdatomic.h>
#include <unistd.h>

#include "common.h"
#include "compat.h"
#include "emu/emu_stat.h"

#define NTASKS 200
atomic_int ncompleted = 0;

nosv_task_t tasks[NTASKS];

static void
task_body(nosv_task_t task)
{
	UNUSED(task);
	sleep_us(500);
	atomic_fetch_add(&ncompleted, 1);
}

int
main(void)
{
	nosv_init();

	nosv_task_type_t task_type;
	nosv_type_init(&task_type, task_body, NULL, NULL, "task", NULL, NULL, 0);

	for (int i = 0; i < NTASKS; i++)
		nosv_create(&tasks[i], task_type, 0, 0);

	for (int i = 0; i < NTASKS; i++)
		nosv_submit(tasks[i], 0);

	while (atomic_load(&ncompleted) != NTASKS)
		sleep_us(1000);

	for (int i = 0; i < NTASKS; i++)
		nosv_destroy(tasks[i], 0);

	nosv_type_destroy(task_type, 0);

	nosv_shutdown();

	return 0;
}
