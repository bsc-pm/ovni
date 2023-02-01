/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "loom.h"

#include <string.h>
#include "cpu.h"
#include "proc.h"
#include "thread.h"
#include "uthash.h"
#include "utlist.h"

static void
set_hostname(char host[PATH_MAX], const char name[PATH_MAX])
{
	/* Copy until dot or end */
	int i;
	for (i = 0; i < PATH_MAX - 1; i++) {
		if (name[i] == '.' || name[i] == '\0')
			break;

		host[i] = name[i];
	}

	host[i] = '\0';
}

static int
has_prefix(const char *path, const char *prefix)
{
	if (strncmp(path, prefix, strlen(prefix)) != 0)
		return 0;

	return 1;
}

int
loom_matches(const char *path)
{
	return has_prefix(path, "loom.");
}

int
loom_init_begin(struct loom *loom, const char *name)
{
	memset(loom, 0, sizeof(struct loom));

	char prefix[] = "loom.";
	if (!has_prefix(name, prefix)) {
		err("loom name must start with '%s': %s", prefix, name);
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

	cpu_init_begin(&loom->vcpu, -1);

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

	return 0;
}

