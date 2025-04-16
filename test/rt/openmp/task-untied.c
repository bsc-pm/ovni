/* Copyright (c) 2025 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

/*
 * Ensures that we can emit a sequence of PPx, PPe, PPx, PPe events for the same
 * OpenMP task to execute it again.
 *
 * See: https://gitlab.pm.bsc.es/rarias/ovni/-/issues/208
 */

int
main(void)
{
	#pragma omp parallel
	#pragma omp master
	#pragma omp task untied
	{
		for (int i = 0; i < 10000; i++) {
			#pragma omp taskyield
		}
	}
}
