/* Copyright (c) 2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "common.h"
#include "compat.h"
#include "ovni.h"
#include <stdio.h>
#include <stdlib.h>

enum { INDEX = 0, RUN = 1 };

#pragma oss task label("fibonacci")
static void
fib2(long index, long *res, int run)
{
	if (index <= 1) {
		*res = index;
	} else {
		ovni_mark_push(RUN, run);
		ovni_mark_push(INDEX, index);

		long a, b;
		fib2(index-1, &a, run);
		fib2(index-2, &b, run);
		#pragma oss taskwait
		*res = a + b;

		ovni_mark_pop(INDEX, index);
		ovni_mark_pop(RUN, run);
	}
}

int
main(void)
{
	ovni_mark_type(INDEX, OVNI_MARK_STACK, "Fibonacci index");
	ovni_mark_type(RUN, OVNI_MARK_STACK, "Fibonacci run");

	long res = 0;
	for (int i = 1; i <= 10; i++) {
		fib2(12, &res, i);
		#pragma oss taskwait
	}

	#pragma oss taskwait
	return 0;
}
