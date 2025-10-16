/* Copyright (c) 2025 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

/* This test checks the correct emulation of a process that uses
 * NOSV_ATTACH/DETACH_INSTRUMENT flags to instrument a pthread
 * */

#include <nosv.h>
#include <nosv/compat.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>

#include "common.h"
#include "compat.h"
#include "ovni.h"

atomic_int completed = 0;

static void *
thread(void *arg)
{
	UNUSED(arg);

	if (!ovni_thread_isready())
		die("nosv_pthread_create: thread not instrumented");

	sleep_us(100);

	atomic_store(&completed, 1);

	return NULL;
}


int main(void)
{
	int rc;
	pthread_t pthread;

	if (nosv_init() != 0)
		die("nosv_init failed");

	if ((rc = nosv_pthread_create(&pthread, NULL, thread, NULL)))
		die("nosv_pthread_create failed: Thread main: %s\n", strerror(rc));

	while (!atomic_load(&completed))
		sleep_us(1000);

	// Wait for thread to detach and end completely
	// Since, we don't have nosv_join yet we just sleep and pray
	sleep_us(200);

	if (nosv_shutdown() != 0)
		die("nosv_shutdown failed");

	return 0;
}
