/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

/* Spawn a task from an external thread that calls some nanos6
 * functions. The external thread must be paused when the task pauses
 * the execution. This emulates the same behavior as TAMPI when using a
 * polling task. */

#define _DEFAULT_SOURCE

#include <nanos6.h>
#include <nanos6/library-mode.h>
#include <pthread.h>
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

/* Call the nanos6_spawn_function from an external thread without
 * instrumentation. */
static void *
spawn(void *arg)
{
	/* Here Nanos6 will try to write events in the thread stream,
	 * but the thread has not been initialized so it will abort */
	nanos6_spawn_function(polling_func, arg,
			complete_func, NULL, "polling_task");

	/* Not reached */
	return NULL;
}

int
main(void)
{
	pthread_t th;
	double *T = malloc(sizeof(double));
	if (T == NULL)
		die("malloc failed:");

	*T = 100.0;

	if (pthread_create(&th, NULL, spawn, &T) != 0)
		die("pthread_create failed:");

	if (pthread_join(th, NULL) != 0)
		die("pthread_join failed:");

	#pragma oss task label("dummy_task")
	dummy_work(*T);

	#pragma oss taskwait

	while (!complete)
		sleep_us(100);

	free(T);

	return 0;
}
