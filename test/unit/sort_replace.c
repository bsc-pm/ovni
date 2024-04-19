/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "emu/sort.h"

static int64_t
randint(void)
{
	return rand() % 1000;
}

static int
cmp_int64(const void *a, const void *b)
{
	int64_t aa = *(const int64_t *) a;
	int64_t bb = *(const int64_t *) b;

	if (aa < bb)
		return -1;
	else if (aa > bb)
		return +1;
	else
		return 0;
}

static void
test_case(int64_t n, int64_t run)
{
	srand(run);
	int64_t *arr = calloc(n, sizeof(int64_t));

	if (arr == NULL)
		die("calloc failed:");

	for (int64_t i = 0; i < n; i++)
		arr[i] = randint();

	qsort(arr, n, sizeof(int64_t), cmp_int64);

	int64_t *copy = calloc(n, sizeof(int64_t));

	if (copy == NULL)
		die("calloc failed:");

	memcpy(copy, arr, n * sizeof(int64_t));

	int64_t iold = rand() % n;
	int64_t old = arr[iold];
	int64_t new = randint();

	/* Ensure old != new */
	while (old == new)
		new = randint();

	dbg("-- CASE run=%lld n=%lld iold=%lld old=%lld new=%lld\n",
			run, n, iold, old, new);

	dbg("Contents before sort: ");
	for (int64_t i = 0; i < n; i++) {
		dbg("i=%lld, arr[i]=%lld, copy[i]=%lld\n",
				i, arr[i], copy[i]);
	}

	sort_replace(arr, n, old, new);

	copy[iold] = new;
	qsort(copy, n, sizeof(int64_t), cmp_int64);

	dbg("Contents after sort: ");
	for (int64_t i = 0; i < n; i++) {
		dbg("i=%lld, arr[i]=%lld, copy[i]=%lld\n",
				i, arr[i], copy[i]);
	}

	if (memcmp(arr, copy, n * sizeof(int64_t)) == 0)
		return;

	die("mismatch");
}

static void
test_sort_replace(void)
{
	int64_t nmin = 2;
	int64_t nmax = 300;
	int64_t nstep = 5;
	int64_t nrun = 50;

	srand(123);
	for (int64_t n = nmin; n <= nmax; n += nstep) {
		for (int64_t run = 0; run < nrun; run++)
			test_case(n, run);
		err("n = %lld OK", n);
	}
}

int
main(void)
{
	test_sort_replace();

	err("OK\n");

	return 0;
}
