/* Copyright (c) 2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdlib.h>
#include "compat.h"
#include "instr.h"
#include "instr_openmp.h"

#define N 30

int
main(void)
{
	instr_start(0, 1);
	instr_openmp_init();

	for (uint32_t type = 1; type <= N; type++)
		instr_openmp_type_create(type, "ws");

	for (uint32_t type = 1; type <= N; type++) {
		instr_openmp_ws_enter(type);
		sleep_us(100);
	}

	for (uint32_t type = N; type > 0; type--) {
		instr_openmp_ws_exit(type);
		sleep_us(100);
	}

	instr_end();

	return 0;
}
