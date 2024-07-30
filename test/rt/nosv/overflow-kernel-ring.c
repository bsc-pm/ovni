/* Copyright (c) 2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <nosv.h>
#include <nosv/affinity.h>
#include <stdatomic.h>
#include <unistd.h>

#include "common.h"
#include "compat.h"

int done = 0;

/* https://gitlab.bsc.es/nos-v/nos-v/-/blob/master/src/include/instr.h?ref_type=heads#L542 */
#define NOSV_INSTR_KBUFLEN (4UL * 1024UL * 1024UL) // 4 MB

static void
task_run(nosv_task_t task)
{
	UNUSED(task);

	/* Cause enough context switches for the kernel ring to be full,
	 * which wouldn't fail if nosv_waitfor() flushes the kernel
	 * events. */

	unsigned long fill = 3 * NOSV_INSTR_KBUFLEN / 4;
	unsigned long n = fill / 16;

	struct timespec one_ns = { .tv_sec = 0, .tv_nsec = 1 };

	for (unsigned long run = 0; run < 3; run++) {
		info("starting run %lu", run);
		for (unsigned long i = 0; i < n; i++)
			nanosleep(&one_ns, NULL);

		info("starting waitfor");

		if (nosv_waitfor(1, NULL) != 0)
			die("nosv_waitfor failed");

		info("end run %lu", run);
	}

	done = 1;
}

int main(void)
{
	if (nosv_init())
		die("nosv_init failed");

	nosv_task_type_t task_type;
	if (nosv_type_init(&task_type, task_run, NULL, NULL, "task", NULL, NULL, NOSV_TYPE_INIT_NONE))
		die("nosv_type_init failed");

	nosv_task_t task;
	if (nosv_create(&task, task_type, 0, NOSV_CREATE_NONE))
		die("nosv_create failed");

	if (nosv_submit(task, NOSV_SUBMIT_IMMEDIATE))
		die("nosv_submit failed");

	/* Wait for the task to finish */
	while (!done)
		sleep_us(10000);

	if (nosv_destroy(task, NOSV_DESTROY_NONE))
		die("nosv_destroy failed");

	if (nosv_type_destroy(task_type, NOSV_TYPE_DESTROY_NONE))
		die("nosv_type_destroy failed");

	if (nosv_shutdown())
		die("nosv_shutdown failed");

	return 0;
}
