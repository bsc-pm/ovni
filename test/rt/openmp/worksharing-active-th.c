/* Copyright (c) 2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <nosv.h>
#include "ovni.h"

/* Ensure that the thread is paused on nosv_waitfor(), so there is a hole in the
 * OpenMP views, as they track the active thread (in the thread views) and the
 * running thread (in the CPU views). */
int
main(void)
{
	ovni_mark_type(0, OVNI_MARK_STACK, "tracker");

	#pragma omp parallel for num_threads(1)
	for (int i = 0; i < 100; ++i) {
		ovni_mark_push(0, 123);
		/* We should see a hole here */
		nosv_waitfor(10, NULL);
		ovni_mark_pop(0, 123);
	}
}
