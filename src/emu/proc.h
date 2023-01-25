/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef PROC_H
#define PROC_H

struct proc;

#include "thread.h"
#include "parson.h"
#include <stddef.h>

struct proc {
	size_t gindex;
	char id[PATH_MAX];

	int metadata_loaded;
	int pid;
	int index;
	int appid;
	int rank;

	int nthreads;
	struct thread *threads;

	/* Loom list */
	struct proc *lnext;
	struct proc *lprev;

	/* Global list */
	struct proc *gnext;
	struct proc *gprev;

	//struct model_ctx ctx;
	UT_hash_handle hh; /* procs in the loom */
};

int proc_init(struct proc *proc, const char *id, int pid);
int proc_get_pid(struct proc *proc);
int proc_load_metadata(struct proc *proc, JSON_Object *meta);
struct thread *proc_find_thread(struct proc *proc, int tid);

#endif /* PROC_H */
