/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "compat.h"
#include <stdio.h>

/* Adapted from Listing 2 of https://arxiv.org/pdf/2208.06332.pdf */

int
main(void)
{
	int A;
	#pragma oss task out(A)
	A = 1;

	#pragma oss taskiter in(A) out(A)
	for (int i = 0; i < 10; i++) {
		#pragma oss task in(A)
		sleep_us(10 + A);
		#pragma oss task out(A)
		A = A + 1;
	}

	# pragma oss task in(A)
	printf("A=%d\n", A);
}
