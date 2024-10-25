/* Copyright (c) 2023-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "common.h"
#include "compat.h"
#include "instr.h"
#include "ovni.h"

#define N 4

/* Ensures that in the CPU timeline, the order of the CPUs is given by the loom
 * alphanumeric order, when no rank information is present. */
int
main(void)
{
	int rank = atoi(getenv("OVNI_RANK"));
	int nranks = atoi(getenv("OVNI_NRANKS"));

	int cpus[N];

	for (int i = 0; i < N; i++) {
		cpus[i] = rank * N + i;
	}

	char loom[128];
	if (snprintf(loom, 128, "loom.%04d", nranks - rank) >= 128)
		die("snprintf failed");

	ovni_proc_init(1, loom, getpid());
	ovni_thread_init(get_tid());

	for (int i = 0; i < N; i++)
		ovni_add_cpu(i, cpus[i]);

	instr_thread_execute(-1, -1, 0);

	instr_end();

	if (rank == 0) {
		FILE *c = fopen("expected", "w");
		if (c == NULL)
			die("fopen failed:");

		/* The expected order should be increasing loom id but the CPUs
		 * should start from the end */
		for (int i = 0; i < nranks; i++) {
			int k = nranks - 1 - i;
			for (int j = k * N; j < (k + 1) * N; j++) {
				fprintf(c, " CPU %d.%d\n", i, j);
			}
		}

		fclose(c);

		FILE *f = fopen("match.sh", "w");
		if (f == NULL)
			die("fopen failed:");

		fprintf(f, "grep ' CPU' ovni/cpu.row > found\n");
		fprintf(f, "diff -y expected found\n");

		fclose(f);
	}

	return 0;
}
