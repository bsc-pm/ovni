/* Copyright (c) 2022 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#define _GNU_SOURCE
#include <unistd.h>

int
main(void)
{
#pragma oss task for
	for (int i = 0; i < 1024; i++)
		usleep(1000);

	return 0;
}
