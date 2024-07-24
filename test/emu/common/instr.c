/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "instr.h"

int first_clock_set = 0;
int64_t first_clock; /* First clock */
int64_t last_clock; /* Clock from the last event */

int64_t get_clock(void)
{
	last_clock = (int64_t) ovni_clock_now();
	if (first_clock_set == 0) {
		first_clock = last_clock;
		first_clock_set = 1;
	}

	return last_clock;
}

int64_t get_delta(void)
{
	return last_clock - first_clock;
}
