/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "cpu.h"

#include "chan.h"
#include "value.h"
#include "utlist.h"

void
cpu_init(struct cpu *cpu, int i, int phyid, int is_virtual)
{
	memset(cpu, 0, sizeof(struct cpu));

	cpu->i = i;
	cpu->phyid = phyid;
	cpu->is_virtual = is_virtual;
}

void
cpu_set_gindex(struct cpu *cpu, int64_t gindex)
{
	cpu->gindex = gindex;
}

void
cpu_set_name(struct cpu *cpu, int64_t loom)
{
	int n;
	if (cpu->is_virtual)
		n = snprintf(cpu->name, PATH_MAX, "vCPU %ld.*", loom);
	else
		n = snprintf(cpu->name, PATH_MAX, "CPU %ld.%d", loom, cpu->i);

	/* Unlikely */
	if (n >= PATH_MAX)
		die("cpu_set_name: cpu name too long\n");
}

static struct thread *
find_thread(struct cpu *cpu, struct thread *thread)
{
	struct thread *p = NULL;

	DL_FOREACH2(cpu->thread, p, cpu_next)
	{
		if (p == thread)
			return p;
	}

	return NULL;
}

static int
update_cpu(struct cpu *cpu)
{
	struct thread *th = NULL;
	struct thread *th_running = NULL;
	struct thread *th_active = NULL;
	int active = 0, running = 0;

	DL_FOREACH2(cpu->thread, th, cpu_next)
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
	cpu->th_running = th_running;
	cpu->th_active = th_active;

	/* Update nth_running number in the channel */
	struct cpu_chan *chan = &cpu->chan;
	if (chan_set(&chan->nth_running, value_int64(cpu->nth_running)) != 0) {
		err("update_cpu: chan_set failed\n");
		return -1;
	}

	return 0;
}

/* Add the given thread to the list of threads assigned to the CPU */
int
cpu_add_thread(struct cpu *cpu, struct thread *thread)
{
	if (find_thread(cpu, thread) != NULL) {
		err("cpu_add_thread: thread %d already assigned to %s\n",
				thread->tid, cpu->name);
		return -1;
	}

	DL_APPEND2(cpu->thread, thread, cpu_prev, cpu_next);
	cpu->nthreads++;

	update_cpu(cpu);
	return 0;
}
