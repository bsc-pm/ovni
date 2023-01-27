/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef LOOM_H
#define LOOM_H

//struct loom;

#include <stddef.h>
#include <stdint.h>
#include <linux/limits.h>
#include <sys/types.h>
#include "cpu.h"
#include "proc.h"
#include "thread.h"

struct loom {
	size_t gindex;
	int is_init;

	char name[PATH_MAX];
	char hostname[PATH_MAX];
	char *id;

	size_t max_ncpus;
	size_t max_phyid;
	size_t ncpus;
	size_t offset_ncpus;
	int rank_enabled;

	int64_t clock_offset;

	/* Physical CPUs hash table by phyid */
	struct cpu *cpus;

	/* Virtual CPU */
	struct cpu vcpu;

	/* Local list */
	size_t nprocs;
	struct proc *procs;

	/* Global list */
	struct loom *next;
	struct loom *prev;

	//struct model_ctx ctx;
};

int loom_matches(const char *relpath);
int loom_init_begin(struct loom *loom, const char *name);
int loom_init_end(struct loom *loom);
int loom_add_cpu(struct loom *loom, struct cpu *cpu);
int64_t loom_get_gindex(struct loom *loom);
void loom_set_gindex(struct loom *loom, int64_t gindex);
struct cpu *loom_find_cpu(struct loom *loom, int phyid);
void loom_set_vcpu(struct loom *loom, struct cpu *vcpu);
struct cpu *loom_get_vcpu(struct loom *loom);
struct proc *loom_find_proc(struct loom *loom, pid_t pid);
struct thread *loom_find_thread(struct loom *loom, int tid);
int loom_add_proc(struct loom *loom, struct proc *proc);
void loom_sort(struct loom *loom);

#endif /* LOOM_H */
