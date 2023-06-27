/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "compat.h"

int
main(void)
{
	#pragma oss taskloop label("taskloop")
	for (int i = 0; i < 100; i++) {
		sleep_us(100);
	}

	#pragma oss taskwait
	return 0;
}
