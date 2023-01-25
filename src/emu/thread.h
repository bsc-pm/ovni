/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef THREAD_H
#define THREAD_H

struct thread;

#include "cpu.h"
#include "chan.h"
#include "emu_stream.h"
#include <stddef.h>

/* Emulated thread runtime status */
enum thread_state {
	TH_ST_UNKNOWN,
	TH_ST_RUNNING,
	TH_ST_PAUSED,
	TH_ST_DEAD,
	TH_ST_COOLING,
	TH_ST_WARMING,
};

struct thread_chan {
	struct chan cpu_gindex;
	struct chan tid_active;
	struct chan nth_active;
	struct chan state;
};

struct thread {
	size_t gindex; /* In the system */

	char name[PATH_MAX];
	char path[PATH_MAX];
	char relpath[PATH_MAX];

	int tid;
	size_t index; /* In loom */

	/* The process associated with this thread */
	struct proc *proc;

	enum thread_state state;
	int is_running;
	int is_active;

	/* Stream linked to this thread */
	struct emu_stream *stream;

	/* Current cpu, NULL if not unique affinity */
	struct cpu *cpu;

	/* Linked list of threads in each CPU */
	struct thread *cpu_prev;
	struct thread *cpu_next;

	/* Local list */
	struct thread *lprev;
	struct thread *lnext;

	/* Global list */
	struct thread *gnext;
	struct thread *gprev;

	struct thread_chan chan;

	//struct model_ctx ctx;
};

void thread_init(struct thread *thread, struct proc *proc);
int thread_set_state(struct thread *th, enum thread_state state);
int thread_set_cpu(struct thread *th, struct cpu *cpu);
int thread_unset_cpu(struct thread *th);
int thread_migrate_cpu(struct thread *th, struct cpu *cpu);

#endif /* THREAD_H */
