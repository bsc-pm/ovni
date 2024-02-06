/* Copyright (c) 2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

/* This test checks the correct emulation of a process that nests
 * nosv_init/nosv_shutdown calls. This is a common behavior when combining
 * multiple libraries that enable nosv on its own. */

#include <nosv.h>
#include <string.h>
#include <pthread.h>

#include "common.h"
#include "compat.h"
#include "ovni.h"

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

	/* Free the thread */
	ovni_thread_free();
}

static void *
thread(void *arg)
{
	nosv_task_t task;
	intptr_t attach = (intptr_t) arg;

	if (attach) {
		instr_thread_start(-1, -1, 0);
		if (nosv_attach(&task, NULL, "thread", NOSV_ATTACH_NONE) != 0)
			die("nosv_attach failed");
	}

	if (nosv_init() != 0)
		die("nosv_init failed: Thread %s: Call 1",
		    attach ? "attach" : "no-attach");

	if (nosv_init() != 0)
		die("nosv_init failed: Thread %s: Call 2",
		    attach ? "attach" : "no-attach");


	if (nosv_shutdown() != 0)
		die("nosv_shutdown failed: Thread %s: Call 3",
		    attach ? "attach" : "no-attach");

	if (nosv_shutdown() != 0)
		die("nosv_shutdown failed: Thread %s: Call 4",
		    attach ? "attach" : "no-attach");

	if (attach) {
		if (nosv_detach(NOSV_DETACH_NONE) != 0)
			die("nosv_detach failed");
		instr_thread_end();
	}

	return NULL;
}

int main(void)
{
	int rc;
	pthread_t pthread;

	if (nosv_init() != 0)
		die("nosv_init failed: Thread main: Call 1");

	if (nosv_init() != 0)
		die("nosv_init failed: Thread main: Call 2");

	if ((rc = pthread_create(&pthread, NULL, thread, NULL)))
		die("pthread_create failed: Thread main: %s\n", strerror(rc));

	if ((rc = pthread_join(pthread, NULL)))
		die("pthread_join failed: Thread main: %s\n", strerror(rc));

	if ((rc = pthread_create(&pthread, NULL, thread, (void *) 1)))
		die("pthread_create failed: Thread main: %s\n", strerror(rc));

	if ((rc = pthread_join(pthread, NULL)))
		die("pthread_join failed: Thread main: %s\n", strerror(rc));

	if (nosv_shutdown() != 0)
		die("nosv_shutdown failed: Thread main: Call 3");

	if (nosv_init() != 0)
		die("nosv_init failed: Thread main: Call 4)");

	if (nosv_shutdown() != 0)
		die("nosv_shutdown failed: Thread main: Call 5");

	if (nosv_shutdown() != 0)
		die("nosv_shutdown failed: Thread main: Call 6");

	return 0;
}
