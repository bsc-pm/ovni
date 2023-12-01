/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "compat.h"
#include <stdio.h>

/* Adapted from Listing 2 of https://arxiv.org/pdf/2208.06332.pdf */

int
main(void)
{
	int A;
	#pragma oss task out(A) label("init")
	A = 1;

	#pragma oss taskiter in(A) out(A) label("iter")
	for (int i = 0; i < 10; i++) {
		#pragma oss task in(A) label("sleep")
		sleep_us(10 + A);
		#pragma oss task out(A) label ("add")
		A = A + 1;
	}

	# pragma oss task in(A) label("print")
	printf("A=%d\n", A);

	#pragma oss taskwait
	return 0;
}
