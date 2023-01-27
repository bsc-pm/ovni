/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "cpu.h"

#include "chan.h"
#include "value.h"
#include "utlist.h"

static const char chan_fmt[] = "cpu%ld.%s";
static const char *chan_name[] = {
	[CPU_CHAN_NRUN] = "nrunning",
	[CPU_CHAN_PID] = "pid_running",
	[CPU_CHAN_TID] = "tid_running",
	[CPU_CHAN_APPID] = "appid_running",
	[CPU_CHAN_FLUSH] = "flush_running",
};

void
cpu_init_begin(struct cpu *cpu, int phyid)
{
	memset(cpu, 0, sizeof(struct cpu));

	cpu->phyid = phyid;

	err("cpu init %d", phyid);
}

int
cpu_get_phyid(struct cpu *cpu)
{
	return cpu->phyid;
}

void
cpu_set_gindex(struct cpu *cpu, int64_t gindex)
{
	cpu->gindex = gindex;
}

void
cpu_set_name(struct cpu *cpu, const char *name)
{
	if (snprintf(cpu->name, PATH_MAX, "%s", name) >= PATH_MAX)
		die("cpu name too long");
}

int
cpu_init_end(struct cpu *cpu)
{
	if (strlen(cpu->name) == 0) {
		err("cpu name not set");
		return -1;
	}

	if (cpu->gindex < 0) {
		err("cpu gindex not set");
		return -1;
	}

	for (int i = 0; i < CPU_CHAN_MAX; i++) {
		chan_init(&cpu->chan[i], CHAN_SINGLE,
				chan_fmt, cpu->gindex, chan_name[i]);
	}

	chan_prop_set(&cpu->chan[CPU_CHAN_NRUN], CHAN_DUPLICATES, 1);

	cpu->is_init = 1;

	return 0;
}

int
cpu_connect(struct cpu *cpu, struct bay *bay)
{
	if (!cpu->is_init) {
		err("cpu not initialized");
		return -1;
	}

	for (int i = 0; i < CPU_CHAN_MAX; i++) {
		if (bay_register(bay, &cpu->chan[i]) != 0) {
			err("bay_register failed");
			return -1;
		}
	}

	return 0;
}

static struct thread *
find_thread(struct cpu *cpu, struct thread *thread)
{
	struct thread *p = NULL;

	/* TODO use hash table */
	DL_FOREACH2(cpu->threads, p, cpu_next)
	{
		if (p == thread)
			return p;
	}

	return NULL;
}

int
cpu_update(struct cpu *cpu)
{
	struct thread *th = NULL;
	struct thread *th_running = NULL;
	struct thread *th_active = NULL;
	int active = 0, running = 0;

	DL_FOREACH2(cpu->threads, th, cpu_next)
	{
		if (th->state == TH_ST_RUNNING) {
			th_running = th;
			running++;
			th_active = th;
			active++;
		} else if (th->state == TH_ST_COOLING || th->state == TH_ST_WARMING) {
			th_active = th;
			active++;
		}
	}

	cpu->nth_running = running;
	cpu->nth_active = active;

	if (running == 1)
		cpu->th_running = th_running;
	else
		cpu->th_running = NULL;

	if (active == 1)
		cpu->th_active = th_active;
	else
		cpu->th_active = NULL;

	/* Update nth_running number in the channel */
	struct chan *nrun = &cpu->chan[CPU_CHAN_NRUN];
	if (chan_set(nrun, value_int64(cpu->nth_running)) != 0) {
		err("chan_set nth_running failed");
		return -1;
	}

	return 0;
}

/* Add the given thread to the list of threads assigned to the CPU */
int
cpu_add_thread(struct cpu *cpu, struct thread *thread)
{
	if (find_thread(cpu, thread) != NULL) {
		err("thread %d already assigned to %s",
				thread->tid, cpu->name);
		return -1;
	}

	DL_APPEND2(cpu->threads, thread, cpu_prev, cpu_next);
	cpu->nthreads++;

	if (cpu_update(cpu) != 0) {
		err("cpu_update failed");
		return -1;
	}

	return 0;
}

int
cpu_remove_thread(struct cpu *cpu, struct thread *thread)
{
	struct thread *t = find_thread(cpu, thread);

	/* Not found, abort */
	if (t == NULL) {
		err("cannot remove missing thread %d from cpu %s\n",
				thread->tid, cpu->name);
		return -1;
	}

	DL_DELETE2(cpu->threads, thread, cpu_prev, cpu_next);
	cpu->nthreads--;

	if (cpu_update(cpu) != 0) {
		err("cpu_update failed");
		return -1;
	}

	return 0;
}

int
cpu_migrate_thread(struct cpu *cpu, struct thread *thread, struct cpu *newcpu)
{
	if (cpu_remove_thread(cpu, thread) != 0) {
		err("cannot remove thread %d from %s",
				thread->tid, cpu->name);
		return -1;
	}

	if (cpu_add_thread(newcpu, thread) != 0) {
		err("cannot add thread %d to %s",
				thread->tid, cpu->name);
		return -1;
	}

	return 0;
}
