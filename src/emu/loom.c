/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "loom.h"

void
loom_init(struct loom *loom)
{
	memset(loom, 0, sizeof(struct loom));
}

void
loom_set_gindex(struct loom *loom, int64_t gindex)
{
	loom->gindex = gindex;
}

void
loom_set_cpus(struct loom *loom, struct cpu *cpus, size_t ncpus)
{
	loom->ncpus = ncpus;
	loom->cpu = cpus;
}

void
loom_set_vcpu(struct loom *loom, struct cpu *vcpu)
{
	loom->vcpu = vcpu;
}

struct emu_proc *
loom_find_proc(struct emu_loom *loom, pid_t pid)
{
	for (struct emu_proc *proc = loom->procs; proc; proc = proc->lnext) {
		if (proc->pid == pid)
			return proc;
	}
	return NULL;
}
