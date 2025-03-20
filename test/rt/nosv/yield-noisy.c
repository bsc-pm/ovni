/* Copyright (c) 2025 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <nosv.h>
#include <nosv/alpi.h>
#include <stdatomic.h>
#include <unistd.h>
#include <stdlib.h>

#include "common.h"
#include "compat.h"

#define NITERS 1000L

atomic_int ncompleted = 0;

static void
busywait(long iter)
{
	for (volatile long i = 0; i < iter; i++)
		;
}

static void
task_body(nosv_task_t task)
{
	UNUSED(task);

	/* Yield a lot of times to try to generate several events */
	for (long i = 0; i < NITERS; i++) {
		nosv_yield(0);
		busywait(10000L);
	}
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

	uint64_t ncpus;

	if (alpi_cpu_count(&ncpus))
		die("alpi_cpu_count failed");

	nosv_task_t *tasks = calloc((size_t) ncpus, sizeof(nosv_task_t));
	if (tasks == NULL)
		die("calloc failed:");

	int ntasks = (int) ncpus;
	info("ntasks = %d", ntasks);

	FILE *f = fopen("vars.sh", "w");
	if (f == NULL)
		die("fopen failed:");

	fprintf(f, "nyields=%ld\n", (long) ntasks * NITERS);
	fclose(f);

	nosv_task_type_t task_type;
	nosv_type_init(&task_type, task_body, NULL, task_done, "task", NULL, NULL, 0);

	for (int i = 0; i < ntasks; i++)
		nosv_create(&tasks[i], task_type, 0, 0);

	for (int i = 0; i < ntasks; i++)
		nosv_submit(tasks[i], 0);

	while (atomic_load(&ncompleted) != ntasks)
		sleep_us(1000);

	for (int i = 0; i < ntasks; i++)
		nosv_destroy(tasks[i], 0);

	nosv_type_destroy(task_type, 0);

	free(tasks);

	nosv_shutdown();

	return 0;
}
