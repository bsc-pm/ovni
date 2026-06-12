/* Copyright (c) 2026 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <nosv.h>
#include <stdatomic.h>
#include <unistd.h>

#include "common.h"

#define OK(x) do { int ret = x; if (ret != 0) { die(#x " failed with ret=%d", ret); } } while (0)

nosv_task_t task_attach;
atomic_int completed = 0;

static void
run_delay(nosv_task_t task)
{
	(void) task;
	nosv_waitfor(100000000ULL, NULL); /* 100ms */
}

static void
complete(nosv_task_t task)
{
	(void) task;
	atomic_fetch_add(&completed, 1);
}

int
main(void)
{
	nosv_task_t task1;
	const int ntasks = 128;
	nosv_task_t tasks[ntasks];

	OK(nosv_init());
	OK(nosv_attach(&task_attach, /* affinity */ NULL, NULL, NOSV_ATTACH_NONE));

	/* Now we are inside nOS-V */

	nosv_task_type_t task_type1;
	OK(nosv_type_init(&task_type1, &run_delay, NULL, &complete, NULL, NULL, NULL, NOSV_TYPE_INIT_NONE));

	/* Check join */
	OK(nosv_create(&task1, task_type1, sizeof(int), NOSV_CREATE_JOINABLE));
	OK(nosv_submit(task1, NOSV_SUBMIT_NONE));
	OK(nosv_join(task1, NULL, (uint64_t)-1));

	/* Check join and timeout */
	OK(nosv_create(&task1, task_type1, sizeof(int), NOSV_CREATE_JOINABLE));
	OK(nosv_submit(task1, NOSV_SUBMIT_NONE));
	OK(nosv_join(task1, NULL, 100ULL) != NOSV_ERR_TIMEOUT);
	OK(nosv_join(task1, NULL, 10000000000ULL)); //10s

	/* Check join (KEEP_ALIVE) */
	OK(nosv_create(&task1, task_type1, sizeof(int), NOSV_CREATE_JOINABLE));
	OK(nosv_submit(task1, NOSV_SUBMIT_NONE));
	OK(nosv_wait(task1, (uint64_t)-1));
	OK(nosv_destroy(task1, NOSV_DESTROY_NONE));

	/* Check join_all */
	for (int i = 0; i < ntasks; i++)
		OK(nosv_create(&tasks[i], task_type1, sizeof(int), NOSV_CREATE_JOINABLE));
	for (int i = 0; i < ntasks; i++)
		OK(nosv_submit(tasks[i], NOSV_SUBMIT_NONE));
	OK(nosv_join_all(tasks, NULL, ntasks, (uint64_t)-1));

	/* Check join_all and timeout */
	for (int i = 0; i < ntasks; i++)
		OK(nosv_create(&tasks[i], task_type1, sizeof(int), NOSV_CREATE_JOINABLE));
	for (int i = 0; i < ntasks; i++)
		OK(nosv_submit(tasks[i], NOSV_SUBMIT_NONE));
	OK(nosv_join_all(tasks, NULL, ntasks, 100ULL) != NOSV_ERR_TIMEOUT);
	OK(nosv_join_all(tasks, NULL, ntasks, 10000000000ULL)); /* 10s */

	/* Check try_join */
	OK(nosv_create(&task1, task_type1, sizeof(int), NOSV_CREATE_JOINABLE));
	OK(nosv_submit(task1, NOSV_SUBMIT_NONE));
	int joined = 0;
	do {
		nosv_waitfor(1000000ULL, NULL); /* 1ms */
		joined = nosv_join(task1, NULL, 0);
	} while (joined == NOSV_ERR_TIMEOUT);

	OK(nosv_type_destroy(task_type1, NOSV_TYPE_DESTROY_NONE));

	OK(nosv_detach(NOSV_DETACH_NONE));
	OK(nosv_shutdown());

	return 0;
}
