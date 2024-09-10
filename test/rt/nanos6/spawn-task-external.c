/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

/* Spawn a task from an external thread that calls some nanos6
 * functions. The external thread must be paused when the task pauses
 * the execution. This emulates the same behavior as TAMPI when using a
 * polling task. */

#define _GNU_SOURCE

#include <nanos6.h>
#include <nanos6/library-mode.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>

#include "common.h"
#include "compat.h"
#include "ovni.h"

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

static inline void
instr_thread_start(int32_t cpu, int32_t creator_tid, uint64_t tag)
{
	ovni_thread_init(get_tid());

	struct ovni_ev ev = {0};

	ovni_ev_set_mcv(&ev, "OHx");
	ovni_ev_set_clock(&ev, ovni_clock_now());
	ovni_payload_add(&ev, (uint8_t *) &cpu, sizeof(cpu));
	ovni_payload_add(&ev, (uint8_t *) &creator_tid, sizeof(creator_tid));
	ovni_payload_add(&ev, (uint8_t *) &tag, sizeof(tag));
	ovni_ev_emit(&ev);

	/* Flush the events to disk after creating the thread */
	ovni_flush();
}

static inline void
instr_thread_end(void)
{
	struct ovni_ev ev = {0};

	ovni_ev_set_mcv(&ev, "OHe");
	ovni_ev_set_clock(&ev, ovni_clock_now());
	ovni_ev_emit(&ev);

	/* Flush the events to disk before killing the thread */
	ovni_flush();

	/* Finish the thread */
	ovni_thread_free();
}

/* Call the nanos6_spawn_function from an external thread */
static void *
spawn(void *arg)
{
	/* Inform ovni of this external thread */
	instr_thread_start(-1, -1, 0);

	nanos6_spawn_function(polling_func, arg,
			complete_func, NULL, "polling_task");

	/* Then inform that the thread finishes */
	instr_thread_end();

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

	if (pthread_create(&th, NULL, spawn, T) != 0)
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
