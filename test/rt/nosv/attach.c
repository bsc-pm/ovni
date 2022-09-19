/* Copyright (c) 2022 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#define _DEFAULT_SOURCE

#include <unistd.h>
#include <nosv.h>

#include "common.h"

int main(void)
{
	nosv_init();

	nosv_task_type_t type;
	if(nosv_type_init(&type, NULL, NULL, NULL, "adopted", NULL,
				NULL, NOSV_TYPE_INIT_EXTERNAL) != 0)
		die("nosv_type_init failed\n");

	nosv_task_t task;
	if(nosv_attach(&task, type, 0, NULL, 0) != 0)
		die("nosv_attach failed\n");

	usleep(100);

	if(nosv_detach(0) != 0)
		die("nosv_detach failed\n");

	if(nosv_type_destroy(type, 0) != 0)
		die("nosv_type_destroy failed\n");

	if(nosv_shutdown() != 0)
		die("nosv_shutdown failed\n");

	return 0;
}
