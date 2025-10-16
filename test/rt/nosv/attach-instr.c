/* Copyright (c) 2025 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

/* This test checks the correct emulation of a process that uses
 * NOSV_ATTACH/DETACH_INSTRUMENT flags to instrument a pthread
 * */

#include <nosv.h>
#include <string.h>
#include <pthread.h>

#include "common.h"
#include "compat.h"
#include "ovni.h"

static void *
thread(void *arg)
{
	UNUSED(arg);

	nosv_task_t task;
	if (nosv_attach(&task, NULL, "thread", NOSV_ATTACH_INSTRUMENT) != 0)
		die("nosv_attach instrument failed");

	sleep_us(100);

	if (nosv_detach(NOSV_DETACH_INSTRUMENT) != 0)
		die("nosv_detach instrument failed");

	return NULL;
}

int main(void)
{
	int rc;
	pthread_t pthread;

	if (nosv_init() != 0)
		die("nosv_init failed");

	if ((rc = pthread_create(&pthread, NULL, thread, NULL)))
		die("pthread_create failed: %s\n", strerror(rc));

	if ((rc = pthread_join(pthread, NULL)))
		die("pthread_join failed: %s\n", strerror(rc));

	if (nosv_shutdown() != 0)
		die("nosv_shutdown failed");

	return 0;
}
