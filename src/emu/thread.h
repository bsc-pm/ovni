/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef THREAD_H
#define THREAD_H

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include "chan.h"
#include "common.h"
#include "extend.h"
#include "uthash.h"
#include "parson.h"
struct bay;
struct cpu;
struct mux;
struct mux_input;
struct pcf;
struct proc;
struct recorder;
struct value;
struct stream;

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

	/* Out of CPU as informed by the kernel */
	int is_out_of_cpu;

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

	/* Metadata */
	JSON_Object *meta;

	struct extend ext;

	UT_hash_handle hh; /* threads in the process */
};

USE_RET int thread_stream_get_tid(struct stream *s);
USE_RET int thread_init_begin(struct thread *thread, int tid);
USE_RET int thread_init_end(struct thread *thread);
USE_RET int thread_load_metadata(struct thread *thread, struct stream *s);
USE_RET int thread_set_state(struct thread *th, enum thread_state state);
USE_RET int thread_set_cpu(struct thread *th, struct cpu *cpu);
USE_RET int thread_unset_cpu(struct thread *th);
USE_RET int thread_migrate_cpu(struct thread *th, struct cpu *cpu);
USE_RET int thread_get_tid(struct thread *thread);
        void thread_set_gindex(struct thread *th, int64_t gindex);
        void thread_set_proc(struct thread *th, struct proc *proc);
USE_RET int thread_connect(struct thread *th, struct bay *bay, struct recorder *rec);
USE_RET int thread_select_active(struct mux *mux, struct value value, struct mux_input **input);
USE_RET int thread_select_running(struct mux *mux, struct value value, struct mux_input **input);
USE_RET int thread_create_pcf_types(struct pcf *pcf);
USE_RET struct pcf_type *thread_get_affinity_pcf_type(struct pcf *pcf);

#endif /* THREAD_H */
