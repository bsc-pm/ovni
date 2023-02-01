/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef THREAD_H
#define THREAD_H

struct thread; /* Needed for cpu */

#include "cpu.h"
#include "proc.h"
#include "chan.h"
#include "bay.h"
#include "uthash.h"
#include "recorder.h"
#include "extend.h"
#include "mux.h"
#include <stddef.h>
#include <linux/limits.h>

/* Emulated thread runtime status */
enum thread_state {
	TH_ST_UNKNOWN,
	TH_ST_RUNNING,
	TH_ST_PAUSED,
	TH_ST_DEAD,
	TH_ST_COOLING,
	TH_ST_WARMING,
};

enum thread_chan {
	TH_CHAN_CPU = 0,
	TH_CHAN_TID,
	TH_CHAN_STATE,
	TH_CHAN_MAX,
};

struct thread {
	int64_t gindex; /* In the system */
	char id[PATH_MAX];

	int is_init;

	int tid;
	size_t index; /* In loom */

	/* The process associated with this thread */
	struct proc *proc;

	enum thread_state state;
	int is_running;
	int is_active;

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

	struct chan chan[TH_CHAN_MAX];

	struct extend ext;

	UT_hash_handle hh; /* threads in the process */
};

int thread_relpath_get_tid(const char *relpath, int *tid);
int thread_init_begin(struct thread *thread, struct proc *proc, const char *relpath);
int thread_init_end(struct thread *thread);
int thread_set_state(struct thread *th, enum thread_state state);
int thread_set_cpu(struct thread *th, struct cpu *cpu);
int thread_unset_cpu(struct thread *th);
int thread_migrate_cpu(struct thread *th, struct cpu *cpu);
int thread_get_tid(struct thread *thread);
void thread_set_gindex(struct thread *th, int64_t gindex);
int thread_connect(struct thread *th, struct bay *bay, struct recorder *rec);

int thread_select_active(struct mux *mux, struct value value, struct mux_input **input);
int thread_select_running(struct mux *mux, struct value value, struct mux_input **input);

#endif /* THREAD_H */
