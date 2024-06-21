/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "cpu.h"
#include <stdio.h>
#include <string.h>
#include "bay.h"
#include "chan.h"
#include "emu_prv.h"
#include "loom.h"
#include "proc.h"
#include "pv/pcf.h"
#include "pv/prv.h"
#include "pv/pvt.h"
#include "recorder.h"
#include "thread.h"
#include "utlist.h"
#include "value.h"

static const char chan_fmt[] = "cpu%"PRIi64".%s";
static const char *chan_name[CPU_CHAN_MAX] = {
	[CPU_CHAN_NRUN]  = "nrunning",
	[CPU_CHAN_PID]   = "pid_running",
	[CPU_CHAN_TID]   = "tid_running",
	[CPU_CHAN_THRUN] = "th_running",
	[CPU_CHAN_THACT] = "th_active",
};

static int chan_type[CPU_CHAN_MAX] = {
	[CPU_CHAN_PID]   = PRV_CPU_PID,
	[CPU_CHAN_TID]   = PRV_CPU_TID,
	[CPU_CHAN_NRUN]  = PRV_CPU_NRUN,
	[CPU_CHAN_THRUN] = -1,
	[CPU_CHAN_THACT] = -1,
};

static long prv_flags[CPU_CHAN_MAX] = {
	[CPU_CHAN_NRUN] = PRV_ZERO,
};

void
cpu_init_begin(struct cpu *cpu, int index, int phyid, int is_virtual)
{
	memset(cpu, 0, sizeof(struct cpu));

	cpu->index = index;
	cpu->phyid = phyid;
	cpu->is_virtual = is_virtual;
	cpu->gindex = -1;

	dbg("cpu init %d", phyid);
}

int
cpu_get_index(struct cpu *cpu)
{
	return cpu->index;
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
cpu_set_loom(struct cpu *cpu, struct loom *loom)
{
	cpu->loom = loom;
}

static int
set_name(struct cpu *cpu)
{
	size_t i = loom_get_gindex(cpu->loom);
	size_t j = cpu_get_phyid(cpu);
	int n;

	if (cpu->is_virtual)
		n = snprintf(cpu->name, PATH_MAX, "vCPU %zu.*", i);
	else
		n = snprintf(cpu->name, PATH_MAX, " CPU %zu.%zu", i, j);

	if (n >= PATH_MAX) {
		err("cpu name too long");
		return -1;
	}

	return 0;
}

int
cpu_init_end(struct cpu *cpu)
{
	if (cpu->gindex < 0) {
		err("cpu gindex not set");
		return -1;
	}

	if (cpu->loom == NULL) {
		err("cpu loom not set");
		return -1;
	}

	if (set_name(cpu) != 0) {
		err("set_name failed");
		return -1;
	}

	for (int i = 0; i < CPU_CHAN_MAX; i++) {
		if (chan_name[i] == NULL) {
			err("chan_name is null");
			return -1;
		}

		chan_init(&cpu->chan[i], CHAN_SINGLE,
				chan_fmt, cpu->gindex, chan_name[i]);
	}

	/* Duplicates may be written when a thread changes the state */
	chan_prop_set(&cpu->chan[CPU_CHAN_NRUN],  CHAN_IGNORE_DUP, 1);
	chan_prop_set(&cpu->chan[CPU_CHAN_TID],   CHAN_IGNORE_DUP, 1);
	chan_prop_set(&cpu->chan[CPU_CHAN_PID],   CHAN_IGNORE_DUP, 1);
	chan_prop_set(&cpu->chan[CPU_CHAN_THRUN], CHAN_IGNORE_DUP, 1);
	chan_prop_set(&cpu->chan[CPU_CHAN_THACT], CHAN_IGNORE_DUP, 1);

	cpu->is_init = 1;

	return 0;
}

int
cpu_connect(struct cpu *cpu, struct bay *bay, struct recorder *rec)
{
	if (!cpu->is_init) {
		err("cpu not initialized");
		return -1;
	}

	/* Get cpu prv */
	struct pvt *pvt = recorder_find_pvt(rec, "cpu");
	if (pvt == NULL) {
		err("cannot find cpu pvt");
		return -1;
	}
	struct prv *prv = pvt_get_prv(pvt);

	for (int i = 0; i < CPU_CHAN_MAX; i++) {
		struct chan *c = &cpu->chan[i];
		if (bay_register(bay, c) != 0) {
			err("bay_register failed");
			return -1;
		}

		long type = chan_type[i];
		if (type < 0)
			continue;

		long row = cpu->gindex;
		long flags = prv_flags[i];
		if (prv_register(prv, row, type, bay, c, flags)) {
			err("prv_register failed");
			return -1;
		}
	}

	return 0;
}

struct pcf_value *
cpu_add_to_pcf_type(struct cpu *cpu, struct pcf_type *type)
{
	return pcf_add_value(type, cpu->gindex + 1, cpu->name);
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

	/* Only virtual cpus can be oversubscribed */
	if (cpu->nth_running > 1 && !cpu->is_virtual) {
		err("physical cpu %s has %d threads running at the same time",
				cpu->name, cpu->nth_running);
		return -1;
	}

	struct value tid_running;
	struct value pid_running;
	struct value gid_running;
	if (running == 1) {
		cpu->th_running = th_running;
		tid_running = value_int64(th_running->tid);
		pid_running = value_int64(th_running->proc->pid);
		gid_running = value_int64(th_running->gindex);
	} else {
		cpu->th_running = NULL;
		tid_running = value_null();
		pid_running = value_null();
		gid_running = value_null();
	}

	if (chan_set(&cpu->chan[CPU_CHAN_TID], tid_running) != 0) {
		err("chan_set tid failed");
		return -1;
	}
	if (chan_set(&cpu->chan[CPU_CHAN_PID], pid_running) != 0) {
		err("chan_set pid failed");
		return -1;
	}
	dbg("cpu%lld sets th_running to %s",
			cpu->gindex, value_str(gid_running));
	if (chan_set(&cpu->chan[CPU_CHAN_THRUN], gid_running) != 0) {
		err("chan_set gid_running failed");
		return -1;
	}

	struct value gid_active;
	if (active == 1) {
		cpu->th_active = th_active;
		gid_active = value_int64(th_active->gindex);
	} else {
		cpu->th_active = NULL;
		gid_active = value_null();
	}

	/* Update nth_running number in the channel */
	if (chan_set(&cpu->chan[CPU_CHAN_NRUN], value_int64(running)) != 0) {
		err("chan_set nth_running failed");
		return -1;
	}
	if (chan_set(&cpu->chan[CPU_CHAN_THACT], gid_active) != 0) {
		err("chan_set gid_active failed");
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
		err("cannot remove missing thread %d from cpu %s",
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

struct chan *
cpu_get_th_chan(struct cpu *cpu)
{
	return &cpu->chan[CPU_CHAN_THRUN];
}
