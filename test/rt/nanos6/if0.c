/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#define _GNU_SOURCE
#include <unistd.h>

int
main(void)
{
	#pragma oss task if (0)
	{
		usleep(1000);
	}
	return 0;
}
