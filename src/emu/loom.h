/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef LOOM_H
#define LOOM_H

struct loom;

#include <stddef.h>
#include <stdint.h>
#include <linux/limits.h>

struct loom {
	size_t gindex;

	char name[PATH_MAX]; /* Loom directory name */
	char path[PATH_MAX];
	char relpath[PATH_MAX];  /* Relative to tracedir */
	char hostname[PATH_MAX];

	size_t max_ncpus;
	size_t max_phyid;
	size_t ncpus;
	size_t offset_ncpus;
	struct cpu *cpu;
	int rank_enabled;

	int64_t clock_offset;

	/* Virtual CPU */
	struct cpu *vcpu;

	/* Local list */
	size_t nprocs;
	struct proc *procs;

	/* Global list */
	struct loom *next;
	struct loom *prev;

	//struct model_ctx ctx;
};

void loom_init(struct loom *loom);

#endif /* LOOM_H */
