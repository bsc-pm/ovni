/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <nosv.h>
#include <unistd.h>

#include "common.h"
#include "compat.h"

int
main(void)
{
	nosv_init();

	nosv_task_t task;
	if (nosv_attach(&task, NULL, NULL, 0) != 0)
		die("nosv_attach failed");

	sleep_us(100);

	if (nosv_detach(0) != 0)
		die("nosv_detach failed");

	if (nosv_shutdown() != 0)
		die("nosv_shutdown failed");

	return 0;
}
