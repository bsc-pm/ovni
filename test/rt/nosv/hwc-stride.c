/* Copyright (c) 2025 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

/*
 * This test creates several tasks that all perform the computation with the
 * same instructions. However, the access to the memory is done differently. The
 * first set of tasks use a stride 1, the next 2, the next 4 and so on until
 * 2^(NSTRIDE-1). This access causes more L3 cache misses, which increases the
 * execution time, typically directly proportional to the stride number.
 *
 * The number of instructions given by PAPI_TOT_INS should remain constant
 * across tasks, but it is expected that PAPI_L3_TCM increases with the
 * stride.
 */

#include <math.h>
#include <nosv.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <unistd.h>

#include "common.h"
#include "compat.h"

#define NTASKS 200
atomic_int ncompleted = 0;

nosv_task_t tasks[NTASKS];

#define NRUNS 2
#define NSTRIDE 4
#define MAXN (256L * 1024L) /* Adjust this for larger L3 */

struct meta {
	long n;
	long stride;
	double *vec;
};

static double
get_time(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double) ts.tv_sec + (double) ts.tv_nsec * 1.0e-9;
}

static void
task_body(nosv_task_t task)
{
	struct meta *meta = nosv_get_task_metadata(task);

	long stride = meta->stride;

	/* Stride access, some computation */
	for (long i = 0; i < stride; i++)
		for (long j = i; j < meta->n; j += stride)
			meta->vec[j] = sqrt(meta->vec[j]);

	atomic_fetch_add(&ncompleted, 1);
}

int
main(void)
{
	nosv_init();

	nosv_task_type_t task_type;
	nosv_type_init(&task_type, task_body, NULL, NULL, "task", NULL, NULL, 0);

	for (int i = 0; i < NTASKS; i++) {
		nosv_create(&tasks[i], task_type, sizeof(struct meta), 0);
		struct meta *meta = nosv_get_task_metadata(tasks[i]);
		meta->n = MAXN;
		meta->vec = calloc(MAXN, sizeof(double));
		for (long i = 0; i < MAXN; i++)
			meta->vec[i] = (double) i;
	}

	fprintf(stderr, "%8s  %8s  %8s\n", "run", "stride", "time");

	/* Repeat for warmup */
	for (int run = 0; run < NRUNS; run++) {
		for (int s = 0; s < NSTRIDE; s++) {
			long stride = 1L << s;

			atomic_store(&ncompleted, 0); /* reset */

			double t0 = get_time();

			for (int i = 0; i < NTASKS; i++) {
				struct meta *meta = nosv_get_task_metadata(tasks[i]);
				meta->stride = stride;
				nosv_submit(tasks[i], 0);
			}

			while (atomic_load(&ncompleted) != NTASKS)
				sleep_us(1000);

			double t1 = get_time();

			printf("%8d  %8ld  %8.3f\n", run, stride, t1 - t0);
		}
	}

	for (int i = 0; i < NTASKS; i++) {
		struct meta *meta = nosv_get_task_metadata(tasks[i]);
		free(meta->vec);
		nosv_destroy(tasks[i], 0);
	}

	nosv_type_destroy(task_type, 0);

	nosv_shutdown();

	return 0;
}
