/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#define _GNU_SOURCE
#include <unistd.h>
#include "compat.h"

int
main(void)
{
	#pragma oss task for
	for (int i = 0; i < 1024; i++)
		sleep_us(1000);

	return 0;
}
