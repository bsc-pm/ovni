/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef LOOM_H
#define LOOM_H

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include "common.h"
#include "cpu.h"
#include "extend.h"
struct proc;

struct loom {
	int64_t gindex;
	int is_init;

	char name[PATH_MAX];
	char hostname[PATH_MAX];
	char *id;

	size_t max_ncpus;
	size_t max_phyid;
	size_t ncpus;
	size_t offset_ncpus;
	int rank_enabled;
	int rank_min;

	int64_t clock_offset;

	/* Physical CPUs hash table by phyid */
	struct cpu *cpus;

	/* Array of CPUs indexed by logic index */
	struct cpu **cpus_array;

	/* Virtual CPU */
	struct cpu vcpu;

	/* Local list */
	size_t nprocs;
	struct proc *procs;

	/* Global list */
	struct loom *next;
	struct loom *prev;

	struct extend ext;
};

USE_RET int loom_matches(const char *relpath);
USE_RET int loom_init_begin(struct loom *loom, const char *name);
USE_RET int loom_init_end(struct loom *loom);
USE_RET int loom_add_cpu(struct loom *loom, struct cpu *cpu);
USE_RET int64_t loom_get_gindex(struct loom *loom);
        void loom_set_gindex(struct loom *loom, int64_t gindex);
USE_RET struct cpu *loom_find_cpu(struct loom *loom, int phyid);
USE_RET struct cpu *loom_get_cpu(struct loom *loom, int index);
        void loom_set_vcpu(struct loom *loom, struct cpu *vcpu);
USE_RET struct cpu *loom_get_vcpu(struct loom *loom);
USE_RET struct proc *loom_find_proc(struct loom *loom, pid_t pid);
USE_RET struct thread *loom_find_thread(struct loom *loom, int tid);
USE_RET int loom_add_proc(struct loom *loom, struct proc *proc);
        void loom_sort(struct loom *loom);

#endif /* LOOM_H */
