/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef CPU_H
#define CPU_H

struct cpu;

#include "loom.h"
#include "thread.h"
#include "chan.h"

struct cpu_chan {
	struct chan pid_running;
	struct chan tid_running;
	struct chan nth_running;
};

struct cpu {
	size_t gindex; /* In the system */
	char name[PATH_MAX];

	/* Logical index: 0 to ncpus - 1 */
	int i;

	/* Physical id: as reported by lscpu(1) */
	int phyid;

	size_t nthreads;
	size_t nth_running;
	size_t nth_active;
	struct thread *thread; /* List of threads assigned to this CPU */
	struct thread *th_running; /* One */
	struct thread *th_active;

	int is_virtual;

	/* Global list */
	struct cpu *next;
	struct cpu *prev;

	/* Channels */
	struct cpu_chan chan;

	//struct model_ctx ctx;
};

void cpu_init(struct cpu *cpu, int i, int phyid, int is_virtual);
void cpu_set_gindex(struct cpu *cpu, int64_t gindex);
void cpu_set_name(struct cpu *cpu, int64_t loom);
int cpu_add_thread(struct cpu *cpu, struct thread *thread);

#endif /* CPU_H */
