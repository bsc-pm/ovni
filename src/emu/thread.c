/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "thread.h"

static void
init_chans(struct thread *th)
{
	struct thread_chan *c = &th->chan;
	char prefix[128];

	if (snprintf(prefix, 128, "sys.thread%lu", th->gindex) >= 128)
		die("snprintf too long");

	chan_init(&c->cpu_gindex, CHAN_SINGLE, "%s.cpu_gindex", prefix);
	chan_init(&c->tid_active, CHAN_SINGLE, "%s.tid_active", prefix);
	chan_init(&c->nth_active, CHAN_SINGLE, "%s.nth_active", prefix);
	chan_init(&c->state, CHAN_SINGLE, "%s.state", prefix);
}

void
thread_init(struct thread *thread, struct proc *proc)
{
	memset(thread, 0, sizeof(struct thread));

	thread->state = TH_ST_UNKNOWN;
	thread->proc = proc;

	init_chans(thread);
}

/* Sets the state of the thread and updates the thread tracking channels */
int
thread_set_state(struct thread *th, enum thread_state state)
{
	/* The state must be updated when a cpu is set */
	if (th->cpu == NULL) {
		die("thread_set_state: thread %d doesn't have a CPU\n", th->tid);
		return -1;
	}

	th->state = state;
	th->is_running = (state == TH_ST_RUNNING) ? 1 : 0;
	th->is_active = (state == TH_ST_RUNNING
					|| state == TH_ST_COOLING
					|| state == TH_ST_WARMING)
					? 1
					: 0;

	struct thread_chan *chan = &th->chan;
	if (chan_set(&chan->state, value_int64(th->state)) != 0) {
		err("thread_set_cpu: chan_set() failed");
		return -1;
	}

	return 0;
}

int
thread_set_cpu(struct thread *th, struct cpu *cpu)
{
	if (cpu == NULL) {
		err("thread_set_cpu: CPU is NULL\n");
		return -1;
	}

	if (th->cpu != NULL) {
		err("thread_set_cpu: thread %d already has a CPU\n", th->tid);
		return -1;
	}

	th->cpu = cpu;

	/* Update cpu channel */
	struct thread_chan *chan = &th->chan;
	if (chan_set(&chan->cpu_gindex, value_int64(cpu->gindex)) != 0) {
		err("thread_set_cpu: chan_set failed\n");
		return -1;
	}

	return 0;
}

int
thread_unset_cpu(struct thread *th)
{
	if (th->cpu == NULL) {
		err("thread_unset_cpu: thread %d doesn't have a CPU\n", th->tid);
		return -1;
	}

	th->cpu = NULL;

	struct thread_chan *chan = &th->chan;
	if (chan_set(&chan->cpu_gindex, value_null()) != 0) {
		err("thread_unset_cpu: chan_set failed\n");
		return -1;
	}

	return 0;
}

int
thread_migrate_cpu(struct thread *th, struct cpu *cpu)
{
	if (th->cpu == NULL) {
		die("thread_migrate_cpu: thread %d doesn't have a CPU\n", th->tid);
		return -1;
	}

	th->cpu = cpu;

	struct thread_chan *chan = &th->chan;
	if (chan_set(&chan->cpu_gindex, value_int64(cpu->gindex)) != 0) {
		err("thread_migrate_cpu: chan_set failed\n");
		return -1;
	}

	return 0;
}
