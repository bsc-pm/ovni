/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef PROC_H
#define PROC_H

/* No loom dependency here */
#include "thread.h"
#include "parson.h"
#include "uthash.h"
#include "extend.h"
#include <stddef.h>
#include <stdint.h>
#include <linux/limits.h>

struct proc {
	int64_t gindex;
	char id[PATH_MAX];
	int is_init;

	int metadata_loaded;
	int pid;
	int index;
	int appid;
	int rank;

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

int proc_relpath_get_pid(const char *relpath, int *pid);
int proc_init_begin(struct proc *proc, const char *id);
int proc_init_end(struct proc *proc);
int proc_get_pid(struct proc *proc);
void proc_set_gindex(struct proc *proc, int64_t gindex);
void proc_set_loom(struct proc *proc, struct loom *loom);
void proc_sort(struct proc *proc);
int proc_load_metadata(struct proc *proc, JSON_Object *meta);
struct thread *proc_find_thread(struct proc *proc, int tid);
int proc_add_thread(struct proc *proc, struct thread *thread);
void proc_sort(struct proc *proc);

#endif /* PROC_H */
