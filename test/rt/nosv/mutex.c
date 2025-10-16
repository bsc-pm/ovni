/* Copyright (c) 2024-2025 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <nosv.h>
#include <nosv/compat.h>
#include <unistd.h>

#include "common.h"
#include "compat.h"

int
main(void)
{
	nosv_task_t task;
	nosv_mutex_t mutex;

	if (nosv_init() != 0)
		die("nosv_init failed");

	if (nosv_attach(&task, NULL, "attached-task", 0) != 0)
		die("nosv_attach failed");

	if (nosv_mutex_init(&mutex, NULL) != 0)
		die("nosv_mutex_init failed");

	if (nosv_mutex_lock(&mutex) != 0)
		die("nosv_mutex_lock failed");

	if (nosv_mutex_trylock(&mutex) != EBUSY)
		die("nosv_mutex_trylock failed");

	if (nosv_mutex_unlock(&mutex) != 0)
		die("nosv_mutex_unlock failed");

	if (nosv_mutex_destroy(&mutex) != 0)
		die("nosv_mutex_destroy failed");


	if (nosv_detach(0) != 0)
		die("nosv_detach failed");

	if (nosv_shutdown() != 0)
		die("nosv_shutdown failed");

	return 0;
}
