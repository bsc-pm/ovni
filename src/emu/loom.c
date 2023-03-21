/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "loom.h"

#include <string.h>
#include "cpu.h"
#include "proc.h"
#include "thread.h"
#include "uthash.h"
#include "utlist.h"

static const char *loom_prefix = "loom.";

static void
set_hostname(char host[PATH_MAX], const char name[PATH_MAX])
{
	/* Skip prefix */
	const char *start = name + strlen(loom_prefix);

	/* Copy until dot or end */
	int i;
	for (i = 0; i < PATH_MAX - 1; i++) {
		if (start[i] == '.' || start[i] == '\0')
			break;

		host[i] = start[i];
	}

	host[i] = '\0';
}

int
loom_matches(const char *path)
{
	return path_has_prefix(path, loom_prefix);
}

int
loom_init_begin(struct loom *loom, const char *name)
{
	memset(loom, 0, sizeof(struct loom));

	if (!path_has_prefix(name, loom_prefix)) {
		err("loom name must start with '%s': %s", loom_prefix, name);
		return -1;
	}

	if (strchr(name, '/') != NULL) {
		err("loom name cannot contain '/': %s", name);
		return -1;
	}

	if (snprintf(loom->name, PATH_MAX, "%s", name) >= PATH_MAX) {
		err("name too long: %s\n", name);
		return -1;
	}

	set_hostname(loom->hostname, loom->name);
	loom->id = loom->name;

	cpu_init_begin(&loom->vcpu, -1, -1, 1);
	cpu_set_loom(&loom->vcpu, loom);

	dbg("creating new loom %s", loom->id);

	return 0;
}

void
loom_set_gindex(struct loom *loom, int64_t gindex)
{
	loom->gindex = gindex;
}

int64_t
loom_get_gindex(struct loom *loom)
{
	return loom->gindex;
}

struct cpu *
loom_find_cpu(struct loom *loom, int phyid)
{
	if (phyid == -1)
		return &loom->vcpu;

	struct cpu *cpu = NULL;
	HASH_FIND_INT(loom->cpus, &phyid, cpu);
	return cpu;
}

struct cpu *
loom_get_cpu(struct loom *loom, int index)
{
	if (index == -1)
		return &loom->vcpu;

	if (index < 0 || (size_t) index >= loom->ncpus) {
		err("cpu index out of bounds");
		return NULL;
	}

	return loom->cpus_array[index];
}

int
loom_add_cpu(struct loom *loom, struct cpu *cpu)
{
	int phyid = cpu_get_phyid(cpu);

	if (phyid < 0) {
		err("cannot use negative cpu phyid %d", phyid);
		return -1;
	}

	if (loom_find_cpu(loom, phyid) != NULL) {
		err("cpu with phyid %d already in loom", phyid);
		return -1;
	}

	if (loom->is_init) {
		err("cannot modify CPUs of initialized loom");
		return -1;
	}

	HASH_ADD_INT(loom->cpus, phyid, cpu);
	//DL_SORT2(loom->cpus, cmp_cpus, lprev, lnext); // Maybe?
	loom->ncpus++;

	cpu_set_loom(cpu, loom);

	return 0;
}

struct cpu *
loom_get_vcpu(struct loom *loom)
{
	return &loom->vcpu;
}

static int
by_pid(struct proc *p1, struct proc *p2)
{
	int id1 = proc_get_pid(p1);
	int id2 = proc_get_pid(p2);

	if (id1 < id2)
		return -1;
	if (id1 > id2)
		return +1;
	else
		return 0;
}

static int
by_phyid(struct cpu *c1, struct cpu *c2)
{
	int id1 = cpu_get_phyid(c1);
	int id2 = cpu_get_phyid(c2);

	if (id1 < id2)
		return -1;
	if (id1 > id2)
		return +1;
	else
		return 0;
}

void
loom_sort(struct loom *loom)
{
	HASH_SORT(loom->procs, by_pid);
	HASH_SORT(loom->cpus, by_phyid);

	for (struct proc *p = loom->procs; p; p = p->hh.next) {
		proc_sort(p);
	}
}

int
loom_init_end(struct loom *loom)
{
	/* Set rank enabled */
	for (struct proc *p = loom->procs; p; p = p->hh.next) {
		if (p->rank >= 0) {
			loom->rank_enabled = 1;
			break;
		}
	}

	/* Populate cpus_array */
	loom->cpus_array = calloc(loom->ncpus, sizeof(struct cpu *));
	if (loom->cpus_array == NULL) {
		err("calloc failed:");
		return -1;
	}
	for (struct cpu *c = loom->cpus; c; c = c->hh.next) {
		int index = cpu_get_index(c);
		if (index < 0 || (size_t) index >= loom->ncpus) {
			err("cpu index out of bounds");
			return -1;
		}

		if (loom->cpus_array[index] != NULL) {
			err("cpu with index %d already taken", index);
			return -1;
		}

		loom->cpus_array[index] = c;
	}

	loom->is_init = 1;

	return 0;
}

struct proc *
loom_find_proc(struct loom *loom, int pid)
{
	struct proc *proc = NULL;
	HASH_FIND_INT(loom->procs, &pid, proc);
	return proc;
}

struct thread *
loom_find_thread(struct loom *loom, int tid)
{
	for (struct proc *p = loom->procs; p; p = p->hh.next) {
		struct thread *thread = proc_find_thread(p, tid);
		if (thread != NULL)
			return thread;
	}

	return NULL;
}

int
loom_add_proc(struct loom *loom, struct proc *proc)
{
	int pid = proc_get_pid(proc);

	if (loom_find_proc(loom, pid) != NULL) {
		err("proc with pid %d already in loom", pid);
		return -1;
	}

	if (loom->is_init) {
		err("cannot modify procs of loom initialized");
		return -1;
	}

	HASH_ADD_INT(loom->procs, pid, proc);
	loom->nprocs++;

	proc_set_loom(proc, loom);

	return 0;
}

