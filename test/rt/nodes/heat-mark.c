/* Copyright (c) 2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "common.h"
#include "compat.h"
#include "ovni.h"
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

enum { ITER = 0, ROW, COL, PRIO, TIME };

long rows;
long cols;

long rbs;
long cbs;
long timesteps;

long nrb;
long ncb;

int use_prio = 0;
int prio_types = 4;

static double
get_time(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double) ts.tv_sec + (double) ts.tv_nsec * 1.0e-9;
}

static void
compute_block(long rstart, long rend, long cstart, long cend, double M[rows][cols])
{
	for (long r = rstart; r <= rend; ++r) {
		for (long c = cstart; c <= cend; ++c) {
			M[r][c] = 0.25*(M[r-1][c] + M[r+1][c] + M[r][c-1] + M[r][c+1]);
		}
	}
}

static void
step(double M[rows][cols], char reps[nrb][ncb], long iter)
{
	for (long R = 1; R < nrb-1; ++R) {
		for (long C = 1; C < ncb-1; ++C) {
			long prio = 0;
			/* Higher number = runs before */
			if (use_prio == 0)
				prio = 1;
			else if (use_prio == 1)
				prio = 1 + nrb * ncb - (R * ncb + C);
			else if (use_prio == 2)
				prio = 1 + nrb + ncb - (R + C);
			else if (use_prio == 3)
				prio = (timesteps + 1) * nrb * ncb - (iter * ncb * nrb + R * ncb + C);

			#pragma oss task label("block computation") \
					in(reps[R-1][C]) in(reps[R+1][C]) \
					in(reps[R][C-1]) in(reps[R][C+1]) \
					priority(prio) \
					inout(reps[R][C])
			{
				ovni_mark_push(ITER, iter);
				ovni_mark_push(ROW, R);
				ovni_mark_push(COL, C);
				ovni_mark_push(PRIO, prio);
				compute_block((R-1)*rbs+1, R*rbs, (C-1)*cbs+1, C*cbs, M);
				ovni_mark_pop(PRIO, prio);
				ovni_mark_pop(COL, C);
				ovni_mark_pop(ROW, R);
				ovni_mark_pop(ITER, iter);
			}
		}
	}
}


static void
solve(double *raw_matrix, long n)
{
	/* Ignore halos */
	double (*matrix)[cols] = (double (*)[cols]) raw_matrix;
	char representatives[nrb][ncb];
	for (long t = 1; t <= n; t++)
		step(matrix, representatives, t);
	#pragma oss taskwait
}

static void
reset(double *matrix)
{
	/* Set all elements to zero */
	memset(matrix, 0, rows * cols * sizeof(double));

	/* Set the left side to 1.0 */
	for (long i = 0; i < rows; i++)
		matrix[i * cols] = 1.0;
}

int
main(void)
{
	ovni_mark_type(ITER, OVNI_MARK_STACK, "Heat iteration");
	ovni_mark_type(ROW, OVNI_MARK_STACK, "Heat row");
	ovni_mark_type(COL, OVNI_MARK_STACK, "Heat col");
	ovni_mark_type(PRIO, OVNI_MARK_STACK, "Heat priority");
	ovni_mark_type(TIME, OVNI_MARK_STACK, "Heat time");

	/* The real number of allocated rows and columns will be bigger */
	long _rows = 1L * 1024L;
	long _cols = _rows;

	rows = _rows + 2;
	cols = _cols + 2;

	rbs = 64L;
	cbs = rbs;
	timesteps = 5L;

	nrb = (rows - 2) / rbs + 2;
	ncb = (cols - 2) / cbs + 2;

	long pagesize = sysconf(_SC_PAGESIZE);
	if (pagesize <= 0)
		die("cannot read pagesize");

	double *matrix;
	int err = posix_memalign((void **) &matrix, pagesize, rows * cols * sizeof(double));
	if (err || matrix == NULL)
		die("posix_memalign failed:");

	/* Do a warmup iteration */
	reset(matrix);
	solve(matrix, 1);

	sleep_us(100000);

	char buf[1024];
	for (int i = 0; i < prio_types; i++) {
		use_prio = i;
		reset(matrix);
		double t0 = get_time();
		solve(matrix, timesteps);
		double t1 = get_time();
		double dt = t1 - t0;

		/* Use label to write a custom string */
		sprintf(buf, "prio=%d took %e s", i, dt);
		ovni_mark_label(TIME, i+1, buf);
		ovni_mark_push(TIME, i+1);
		sleep_us(100000);
		ovni_mark_pop(TIME, i+1);
	}

	free(matrix);

	return 0;
}
