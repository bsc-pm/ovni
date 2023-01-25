/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef PROC_H
#define PROC_H

struct proc;

#include "loom.h"
#include "thread.h"
#include "parson.h"
#include <stddef.h>

struct proc {
	size_t gindex;

	char name[PATH_MAX]; /* Proc directory name */
	char fullpath[PATH_MAX];
	char relpath[PATH_MAX];
	char *id; /* Points to relpath */

	pid_t pid;
	int index;
	int appid;
	int rank;

	struct loom *loom;

	JSON_Value *meta;

	int nthreads;
	struct thread *threads;

	/* Local list */
	struct proc *lnext;
	struct proc *lprev;

	/* Global list */
	struct proc *gnext;
	struct proc *gprev;

	//struct model_ctx ctx;
};

void proc_init(struct proc *proc);

#endif /* PROC_H */
