/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

/* Spawn a task from the main thread that calls some nanos6
 * functions. */

#define _DEFAULT_SOURCE

#include <nanos6.h>
#include <nanos6/library-mode.h>
#include <stdlib.h>
#include <time.h>

#include "common.h"
#include "compat.h"

volatile int complete = 0;

static double
get_time_ms(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double) ts.tv_sec + (double) ts.tv_nsec * 1.0e-9;
}

static void
dummy_work(double ms)
{
	double end = get_time_ms() + ms * 1e-3;
	while (get_time_ms() < end);
}

static void
polling_func(void *arg)
{
	double ms = *((double *) arg);
	double end = get_time_ms() + ms * 1e-3;
	while (get_time_ms() < end) {
		dummy_work(1.0); /* 1 ms */
		nanos6_wait_for(1000UL); /* 1 ms */
	}
}

static void
complete_func(void *arg)
{
	UNUSED(arg);
	complete = 1;
}

int
main(void)
{
	double *T = malloc(sizeof(double));
	if (T == NULL)
		die("malloc failed:");

	*T = 100.0;

	nanos6_spawn_function(polling_func, T,
			complete_func, NULL, "polling_task");

	#pragma oss task label("dummy_task")
	dummy_work(*T);

	#pragma oss taskwait

	while (!complete)
		sleep_us(100);

	free(T);

	return 0;
}
