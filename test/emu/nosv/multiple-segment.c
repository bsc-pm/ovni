/* Copyright (c) 2023-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "common.h"
#include "compat.h"
#include "emu_prv.h"
#include "instr_nosv.h"
#include "ovni.h"

/* Number of CPUs per loom, also number of threads per loom */
#define N 2

/* This test executes two apps in two segments of nOS-V, each segment has 2
 * CPUs, with 2 in total. */
int
main(void)
{
	int rank = atoi(getenv("OVNI_RANK"));
	int nranks = atoi(getenv("OVNI_NRANKS"));

	if (nranks != 4)
		die("this tests requires 4 ranks");

	int app = rank / N;

	char loom[128];
	if (snprintf(loom, 128, "node0.%04d", app) >= 128)
		die("snprintf failed");

	ovni_proc_init(1 + app, loom, getpid());
	ovni_proc_set_rank(rank, nranks);
	ovni_thread_init(get_tid());

	/* Leader of the segment, must emit CPUs */
	if (rank % N == 0) {
		int cpus[N];
		for (int i = 0; i < N; i++) {
			cpus[i] = app * N + i;
			ovni_add_cpu(i, cpus[i]);
			info("adding cpu %d to rank %d", i, rank);
		}
	}

	int nlooms = nranks / N;
	int lcpu = rank % N;

	instr_nosv_init();
	instr_thread_execute(lcpu, -1, 0);

	uint32_t typeid = 1;
	instr_nosv_type_create((int32_t) typeid);
	instr_nosv_task_create(1, typeid);
	instr_nosv_task_execute(1, 0);
	sleep_us(10000);
	instr_nosv_task_end(1, 0);

	instr_end();

	char pname[128];
	sprintf(pname, "check-rank-%d.sh", rank);
	FILE *p = fopen(pname, "w");
	if (p == NULL)
		die("fopen failed:");

	/* Check the value of the rank and app id of each process is
	 * placed in the proper row (keep in mind the virtual CPU). */
	int row = 1 + rank + app;

	/* TODO: we cannot use the delta clock in all the ranks, as it
	 * needs to be corrected to the delta of the first rank. For
	 * now we simply search for the event in the proper row. */
	fprintf(p, "grep '%d:[0-9]*:%d:%d$' ovni/cpu.prv\n",
			row, PRV_NOSV_APPID, 1 + app); /* Starts at 1 */
	fprintf(p, "grep '%d:[0-9]*:%d:%d$' ovni/cpu.prv\n",
			row, PRV_NOSV_RANK, 1 + rank); /* Starts at 1 */

	fclose(p);

	/* Only the rank 0 writes the cpu script */
	if (rank != 0)
		return 0;

	FILE *c = fopen("cpus", "w");
	if (c == NULL)
		die("fopen failed:");

	/* Check that there are exactly 6 rows: two groups of 2 CPUs and
	 * 1 vCPU. */
	for (int l = 0; l < nlooms; l++) {
		for (int i = l * N; i < (l + 1) * N ; i++) {
			fprintf(c, " CPU %d.%d\n", l, i);
		}
		fprintf(c, "vCPU %d.*\n", l);
	}

	fclose(c);

	FILE *f = fopen("check-rows.sh", "w");
	if (f == NULL)
		die("fopen failed:");

	fprintf(f, "grep 'CPU' ovni/cpu.row | diff -y cpus -\n");

	fclose(f);

	return 0;
}
