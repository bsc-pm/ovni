/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef PROC_H
#define PROC_H

#include <limits.h>
#include <stdint.h>
#include "common.h"
#include "extend.h"
#include "uthash.h"
struct loom;
struct stream;
struct thread;

struct proc {
	int64_t gindex;
	char id[PATH_MAX];
	int is_init;

	int pid;
	int index;
	int appid;
	int rank;
	int nranks;

	int nthreads;
	struct thread *threads;

	/* Required to find if a thread belongs to the same loom as a
	 * CPU */
	struct loom *loom;

	/* Loom list */
	struct proc *lnext;
	struct proc *lprev;

	/* Global list */
	struct proc *gnext;
	struct proc *gprev;

	//struct model_ctx ctx;
	UT_hash_handle hh; /* procs in the loom */
	struct extend ext;
};

USE_RET int proc_stream_get_pid(struct stream *s);
USE_RET int proc_init_begin(struct proc *proc, int pid);
USE_RET int proc_init_end(struct proc *proc);
USE_RET int proc_get_pid(struct proc *proc);
        void proc_set_gindex(struct proc *proc, int64_t gindex);
        void proc_set_loom(struct proc *proc, struct loom *loom);
        void proc_sort(struct proc *proc);
USE_RET int proc_load_metadata(struct proc *proc, struct stream *s);
USE_RET struct thread *proc_find_thread(struct proc *proc, int tid);
USE_RET int proc_add_thread(struct proc *proc, struct thread *thread);
        void proc_sort(struct proc *proc);

#endif /* PROC_H */
